import java.io.IOException;
import java.nio.file.Paths;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.jline.utils.Log;
import org.tensorflow.Graph;
import org.tensorflow.framework.initializers.Glorot;
import org.tensorflow.framework.initializers.VarianceScaling;
import org.tensorflow.ndarray.Shape;
import org.tensorflow.op.Ops;
import org.tensorflow.op.core.Placeholder;
import org.tensorflow.types.TFloat32;
import org.tribuo.*;
import org.tribuo.classification.Label;
import org.tribuo.classification.LabelFactory;
import org.tribuo.classification.evaluation.LabelEvaluation;
import org.tribuo.classification.evaluation.LabelEvaluator;
import org.tribuo.data.csv.CSVLoader;
import org.tribuo.datasource.IDXDataSource;
import org.tribuo.evaluation.TrainTestSplitter;
import org.tribuo.interop.tensorflow.*;
import org.tribuo.interop.tensorflow.example.CNNExamples;
import org.tribuo.interop.tensorflow.example.MLPExamples;
import org.tribuo.regression.*;
import org.tribuo.regression.evaluation.*;
import org.tribuo.util.Util;

public class TribuoTF {
    private static String DATASET_PATH = "/home/dipanzan/IdeaProjects/machine-learning-java/dataset/";

    private static Object buildMLPRegressionModel(int epochs) {
        // First we load winequality
        var regressionFactory = new RegressionFactory();
        var regEval = new RegressionEvaluator();
        var csvLoader = new CSVLoader<>(';', regressionFactory);
        DataSource<Regressor> wineSource = null;
        try {
            wineSource = csvLoader.loadDataSource(Paths.get(DATASET_PATH + "winequality-red.csv"), "quality");
        } catch (IOException e) {
            //
        }
        var wineSplitter = new TrainTestSplitter<>(wineSource, 0.7f, 0L);
        var wineTrain = new MutableDataset<>(wineSplitter.getTrain());
        var wineTest = new MutableDataset<>(wineSplitter.getTest());

        Graph wineGraph = new Graph();
// This object is used to write operations into the graph
        var wineOps = Ops.create(wineGraph);
        var wineInputName = "WINE_INPUT";
        long wineNumFeatures = wineTrain.getFeatureMap().size();
        var wineInitializer = new Glorot<TFloat32>(// Initializer distribution
                VarianceScaling.Distribution.TRUNCATED_NORMAL,
                // Initializer seed
                Trainer.DEFAULT_SEED
        );

// The input placeholder that we'll feed the features into
        var wineInput = wineOps.withName(wineInputName).placeholder(TFloat32.class,
                Placeholder.shape(Shape.of(-1, wineNumFeatures)));

// Fully connected layer (numFeatures -> 30)
        var fc1Weights = wineOps.variable(wineInitializer.call(wineOps, wineOps.array(wineNumFeatures, 30L),
                TFloat32.class));
        var fc1Biases = wineOps.variable(wineOps.fill(wineOps.array(30), wineOps.constant(0.1f)));
        var sigmoid1 = wineOps.math.sigmoid(wineOps.math.add(wineOps.linalg.matMul(wineInput, fc1Weights),
                fc1Biases));

// Fully connected layer (30 -> 20)
        var fc2Weights = wineOps.variable(wineInitializer.call(wineOps, wineOps.array(30L, 20L),
                TFloat32.class));
        var fc2Biases = wineOps.variable(wineOps.fill(wineOps.array(20), wineOps.constant(0.1f)));
        var sigmoid2 = wineOps.math.sigmoid(wineOps.math.add(wineOps.linalg.matMul(sigmoid1, fc2Weights),
                fc2Biases));

// Output layer (20 -> 1)
        var outputWeights = wineOps.variable(wineInitializer.call(wineOps, wineOps.array(20L, 1L),
                TFloat32.class));
        var outputBiases = wineOps.variable(wineOps.fill(wineOps.array(1), wineOps.constant(0.1f)));
        var outputOp = wineOps.math.add(wineOps.linalg.matMul(sigmoid2, outputWeights), outputBiases);

// Get the operation name to pass into the trainer
        var wineOutputName = outputOp.op().name();

        var gradAlgorithm = GradientOptimiser.ADAGRAD;
        var gradParams = Map.of("learningRate", 0.1f, "initialAccumulatorValue", 0.01f);

        var wineDenseConverter = new DenseFeatureConverter(wineInputName);
        var wineOutputConverter = new RegressorConverter();

        var wineTrainer = new TensorFlowTrainer<Regressor>(wineGraph,
                wineOutputName,
                gradAlgorithm,
                gradParams,
                wineDenseConverter,
                wineOutputConverter,
                16,   // training batch size
                epochs,  // number of training epochs
                16,   // test batch size of the trained model
                -1    // disable logging of the loss value
        );

        var wineStart = System.currentTimeMillis();
//        var wineModel = (TensorFlowModel<Regressor>) Method.execute(() -> , "wineTrainer.train");
        var wineModel = wineTrainer.train(wineTrain);
        var wineEnd = System.currentTimeMillis();
        System.out.println("Wine quality training took " + Util.formatDuration(wineStart, wineEnd));

        // Now we close the original graph to free the associated native resources.
        // The TensorFlowTrainer keeps a copy of the GraphDef protobuf to rebuild it when necessary.
        wineGraph.close();

//        var wineEvaluation = (RegressionEvaluation) Method.execute(() -> regEval.evaluate(wineModel, wineTest), "wineTrainer.evaluate");
        var wineEvaluation = regEval.evaluate(wineModel, wineTest);
        var dimension = new Regressor("DIM-0", Double.NaN);

        System.out.println(String.format("Wine quality evaluation:%n  RMSE %f%n  MAE %f%n  R^2 %f%n",
                wineEvaluation.rmse(dimension),
                wineEvaluation.mae(dimension),
                wineEvaluation.r2(dimension)));

        return 0;
    }

    private static Object buildModelCNN(int epochs) {
        // Now we load MNIST
        var labelFactory = new LabelFactory();
        var labelEval = new LabelEvaluator();
        IDXDataSource<Label> mnistTrainSource = null;
        IDXDataSource<Label> mnistTestSource = null;
        try {
            mnistTrainSource =
                    new IDXDataSource<>(Paths.get(DATASET_PATH + "train-images-idx3-ubyte.gz"), Paths.get(DATASET_PATH + "train-labels-idx1-ubyte.gz"), labelFactory);
            mnistTestSource =
                    new IDXDataSource<>(Paths.get(DATASET_PATH + "t10k-images-idx3-ubyte.gz"), Paths.get(DATASET_PATH + "t10k-labels-idx1-ubyte.gz"), labelFactory);
        } catch (IOException e) {
            //
        }
        var mnistTrain = new MutableDataset<>(mnistTrainSource);
        var mnistTest = new MutableDataset<>(mnistTestSource);

        var mnistInputName = "MNIST_INPUT";
        var mnistCNNTuple = CNNExamples.buildLeNetGraph(mnistInputName, 28, 255, mnistTrain.getOutputs().size());
        var mnistImageConverter = new ImageConverter(mnistInputName, 28, 28, 1);
        var gradAlgorithm = GradientOptimiser.ADAGRAD;
        var gradParams = Map.of("learningRate", 0.1f, "initialAccumulatorValue", 0.01f);
        var mnistOutputConverter = new LabelConverter();

        var mnistCNNTrainer = new TensorFlowTrainer<Label>(mnistCNNTuple.graphDef,
                mnistCNNTuple.outputName, // the name of the logit operation
                gradAlgorithm,            // the gradient descent algorithm
                gradParams,               // the gradient descent hyperparameters
                mnistImageConverter,      // the input feature converter
                mnistOutputConverter,     // the output label converter
                16, // training batch size
                epochs,  // number of training epochs
                16, // test batch size of the trained model
                -1  // disable logging of the loss value
        );

// Training the model
        var cnnStart = System.currentTimeMillis();
        var cnnModel = mnistCNNTrainer.train(mnistTrain);
        var cnnEnd = System.currentTimeMillis();
        System.out.println("MNIST CNN training took " + Util.formatDuration(cnnStart, cnnEnd));

        var cnnPredictions = cnnModel.predict(mnistTest);
        var cnnEvaluation = labelEval.evaluate(cnnModel, cnnPredictions, mnistTest.getProvenance());
        System.out.println(cnnEvaluation.toString());
        System.out.println(cnnEvaluation.getConfusionMatrix().toString());

        return 0;
    }

    private static Object buildClassificationMLP(int epochs) {
        // Now we load MNIST
        var labelFactory = new LabelFactory();
        var labelEval = new LabelEvaluator();
        IDXDataSource<Label> mnistTrainSource = null;
        IDXDataSource<Label> mnistTestSource = null;
        try {

            mnistTrainSource =
                    new IDXDataSource<>(Paths.get(DATASET_PATH + "train-images-idx3-ubyte.gz"), Paths.get(DATASET_PATH + "train-labels-idx1-ubyte.gz"), labelFactory);
            mnistTestSource =
                    new IDXDataSource<>(Paths.get(DATASET_PATH + "t10k-images-idx3-ubyte.gz"), Paths.get(DATASET_PATH + "t10k-labels-idx1-ubyte.gz"), labelFactory);
        } catch (IOException e) {
            //
        }
        var mnistTrain = new MutableDataset<>(mnistTrainSource);
        var mnistTest = new MutableDataset<>(mnistTestSource);


        var mnistInputName = "MNIST_INPUT";
        var mnistMLPTuple = MLPExamples.buildMLPGraph(
                mnistInputName, // The input placeholder name
                mnistTrain.getFeatureMap().size(), // The number of input features
                new int[]{300, 200, 30}, // The hidden layer sizes
                mnistTrain.getOutputs().size() // The number of output labels
        );
        var mnistDenseConverter = new DenseFeatureConverter(mnistInputName);
        var mnistOutputConverter = new LabelConverter();

        var gradAlgorithm = GradientOptimiser.ADAGRAD;
        var gradParams = Map.of("learningRate", 0.1f, "initialAccumulatorValue", 0.01f);

        var mnistMLPTrainer = new TensorFlowTrainer<Label>(mnistMLPTuple.graphDef,
                mnistMLPTuple.outputName, // the name of the logit operation
                gradAlgorithm,            // the gradient descent algorithm
                gradParams,               // the gradient descent hyperparameters
                mnistDenseConverter,      // the input feature converter
                mnistOutputConverter,     // the output label converter
                16,  // training batch size
                epochs,  // number of training epochs
                16,  // test batch size of the trained model
                -1   // disable logging of the loss value
        );

        var mlpStart = System.currentTimeMillis();
//        var mlpModel = (TensorFlowModel<Label>) Method.execute(() -> mnistMLPTrainer.train(mnistTrain), "mnistMLPTrainer.train");
        var mlpModel = mnistMLPTrainer.train(mnistTrain);
        var mlpEnd = System.currentTimeMillis();
        System.out.println("MNIST MLP training took " + Util.formatDuration(mlpStart, mlpEnd));

//        var mlpEvaluation = (LabelEvaluation) Method.execute(() -> labelEval.evaluate(mlpModel, mnistTest), "mnistMLPTrainer.evaluate");
        var mlpEvaluation = labelEval.evaluate(mlpModel, mnistTest);
        System.out.println(mlpEvaluation.toString());
        System.out.println(mlpEvaluation.getConfusionMatrix().toString());

        return 0;
    }

    public static void main(String[] args) throws IOException {
//        Logger logger = Logger.getLogger(TensorFlowTrainer.class.getName());
//        logger.setLevel(Level.OFF);
//        long before = Method.readEnergy(1);

        int epochs = Integer.parseInt(args[0]);
        Method.execute(() -> buildMLPRegressionModel(epochs), "MLPRegressionModel(train+eval)");
        Method.execute(() -> buildClassificationMLP(epochs), "classificationMLP(train+eval)");
        Method.execute(() -> buildModelCNN(epochs), "modelCNN(train+eval)");

//        long after = Method.readEnergy(1);
//        Method.printEnergyConsumed(before, after, "main");
    }
}