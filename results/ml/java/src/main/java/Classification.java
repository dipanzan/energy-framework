import org.tribuo.*;
import org.tribuo.classification.Label;
import org.tribuo.classification.LabelFactory;
import org.tribuo.classification.dtree.CARTClassificationTrainer;
import org.tribuo.classification.evaluation.LabelEvaluation;
import org.tribuo.classification.evaluation.LabelEvaluator;
import org.tribuo.classification.liblinear.LibLinearClassificationTrainer;
import org.tribuo.classification.sgd.linear.LinearSGDTrainer;
import org.tribuo.classification.sgd.objectives.Hinge;
import org.tribuo.classification.sgd.objectives.LogMulticlass;
import org.tribuo.classification.xgboost.XGBoostClassificationTrainer;
import org.tribuo.data.csv.CSVLoader;
import org.tribuo.evaluation.TrainTestSplitter;
import org.tribuo.math.optimisers.AdaGrad;
import org.tribuo.util.Util;

import java.io.IOException;
import java.nio.file.Paths;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;

public class Classification {
    private static String DATASET_PATH = "/home/dipanzan/IdeaProjects/energy-framework/results/ml/dataset/";

    private static class DataPartition {
        private MutableDataset<Label> train, test;

        public DataPartition(MutableDataset<Label> train, MutableDataset<Label> test) {
            this.train = train;
            this.test = test;
        }
    }

    public static DataPartition loadWeatherData() {
        var labelFactory = new LabelFactory();
        var csvLoader = new CSVLoader<>(labelFactory);

        var rainHeaders = new String[]{"Month", "MinTemp", "MaxTemp", "Rainfall", "WindGustSpeed", "WindSpeed9am",
                "WindSpeed3pm", "Humidity9am", "Humidity3pm", "Pressure9am", "Pressure3pm",
                "Temp9am", "Temp3pm", "RainToday", "WindGustDir_E", "WindGustDir_ENE",
                "WindGustDir_ESE", "WindGustDir_N", "WindGustDir_NE", "WindGustDir_NNE",
                "WindGustDir_NNW", "WindGustDir_NW", "WindGustDir_S", "WindGustDir_SE",
                "WindGustDir_SSE", "WindGustDir_SSW", "WindGustDir_SW", "WindGustDir_W",
                "WindGustDir_WNW", "WindGustDir_WSW", "WindDir9am_E", "WindDir9am_ENE",
                "WindDir9am_ESE", "WindDir9am_N", "WindDir9am_NE", "WindDir9am_NNE", "WindDir9am_NNW",
                "WindDir9am_NW", "WindDir9am_S", "WindDir9am_SE", "WindDir9am_SSE", "WindDir9am_SSW",
                "WindDir9am_SW", "WindDir9am_W", "WindDir9am_WNW", "WindDir9am_WSW", "WindDir3pm_E",
                "WindDir3pm_ENE", "WindDir3pm_ESE", "WindDir3pm_N", "WindDir3pm_NE", "WindDir3pm_NNE",
                "WindDir3pm_NNW", "WindDir3pm_NW", "WindDir3pm_S", "WindDir3pm_SE", "WindDir3pm_SSE",
                "WindDir3pm_SSW", "WindDir3pm_SW", "WindDir3pm_W", "WindDir3pm_WNW", "WindDir3pm_WSW",
                "RainTomorrowN"};

        // This dataset is prepared in the notebook: scikit-learn Classifier - Data Cleanup
        DataSource<Label> weatherSource = null;
        try {
            weatherSource = csvLoader.loadDataSource(Paths.get(DATASET_PATH + "cleanedWeatherAUS.csv"), "RainTomorrowN", rainHeaders);
        } catch (IOException ioex) {
            // oops
        }
        var weatherSplitter = new TrainTestSplitter<>(weatherSource, 0.8, 1L);

        var trainingDataset = new MutableDataset<>(weatherSplitter.getTrain());
        var testingDataset = new MutableDataset<>(weatherSplitter.getTest());

        DataPartition dp = new DataPartition(trainingDataset, testingDataset);
        System.out.println(String.format("Training data size = %d, number of features = %d, number of classes = %d", trainingDataset.size(), trainingDataset.getFeatureMap().size(), trainingDataset.getOutputInfo().size()));
        System.out.println(String.format("Testing data size = %d, number of features = %d, number of classes = %d", testingDataset.size(), testingDataset.getFeatureMap().size(), testingDataset.getOutputInfo().size()));

        return dp;
    }

    public static void weatherClassificaiton(DataPartition dp, int epochs) throws Exception {

        List<Trainer> trainers = List.of(
                new LinearSGDTrainer(
                        new Hinge(),
                        new AdaGrad(0.1, 0.1), // SGD.getLinearDecaySGD(0.01),
                        100,
                        Trainer.DEFAULT_SEED
                ),
                new LibLinearClassificationTrainer(),
                new CARTClassificationTrainer(),
                new XGBoostClassificationTrainer(100)
        );

        List<Model> models = trainers.stream()
                .map(t -> (Model) Method.execute(() -> train(t.toString(), t, dp.train), t.getClass().getSimpleName() + ".train()"))
                .collect(Collectors.toList());

        List<Object> evaluations = models.stream()
                .map(m -> Method.execute(() -> evaluate(m, dp.test), m.getClass().getSimpleName() + ".evaluate()"))
                .collect(Collectors.toList());
    }

    public static DataPartition loadIrisData() throws Exception {
        var labelFactory = new LabelFactory();
        var csvLoader = new CSVLoader<>(labelFactory);
        var irisHeaders = new String[]{"sepalLength", "sepalWidth", "petalLength", "petalWidth", "species"};
        var irisesSource = csvLoader.loadDataSource(Paths.get(DATASET_PATH + "bezdekIris.data"), "species", irisHeaders);
        var irisSplitter = new TrainTestSplitter<>(irisesSource, 0.7, 1L);

        var trainingDataset = new MutableDataset<>(irisSplitter.getTrain());
        var testingDataset = new MutableDataset<>(irisSplitter.getTest());
        System.out.println(String.format("Training data size = %d, number of features = %d, number of classes = %d", trainingDataset.size(), trainingDataset.getFeatureMap().size(), trainingDataset.getOutputInfo().size()));
        System.out.println(String.format("Testing data size = %d, number of features = %d, number of classes = %d", testingDataset.size(), testingDataset.getFeatureMap().size(), testingDataset.getOutputInfo().size()));

        DataPartition dp = new DataPartition(trainingDataset, testingDataset);
        return dp;
    }

    public static void irisClassificaiton(DataPartition dp, int epochs) {
        Trainer<Label> sgdTrainer = new LinearSGDTrainer(new LogMulticlass(), new AdaGrad(1.0, 0.1), epochs, Trainer.DEFAULT_SEED);

        // Model<Label> sgdModel = lrTrainer.train(trainingDataset);
        Model sgdModel = (Model) Method.execute(() -> train(sgdTrainer.getClass().getSimpleName(), sgdTrainer, dp.train),
                sgdTrainer.getClass().getSimpleName() + "-train()");

        var evaluator = new LabelEvaluator();
        var evaluation = (LabelEvaluation) Method.execute(() -> evaluator.evaluate(sgdModel, dp.test), evaluator.getClass().getSimpleName() + "-evaluate()");
        System.out.println(evaluation.toString());
        System.out.println(evaluation.getConfusionMatrix().toString());
    }


    public static Model train(String name, Trainer trainer, Dataset<Label> trainData) {
        var startTime = System.currentTimeMillis();
        var model = trainer.train(trainData);
        var endTime = System.currentTimeMillis();
        System.out.println("Training " + name + " took " + Util.formatDuration(startTime, endTime));
        return model;
    }

    public static Object evaluate(Model model, Dataset<Label> testData) {
        var eval = new LabelEvaluator();
        var evaluation = eval.evaluate(model, testData);
        System.out.println(evaluation.toString());
        System.out.println(evaluation.getConfusionMatrix().toString());
        return 0;
    }

    public static void main(String[] args) throws Exception {
        // Turn off that SGD logging - it effects performance
        var logger = Logger.getLogger(org.tribuo.common.sgd.AbstractSGDTrainer.class.getName());
        logger.setLevel(Level.OFF);

        int epochs = Integer.parseInt(args[0]);

        DataPartition lwd = (DataPartition) Method.execute(Classification::loadWeatherData, "loadWeatherData.testTrainSplit()");
        weatherClassificaiton(lwd, epochs);
//        irisClassificaiton(loadIrisData(), epochs);


    }
}