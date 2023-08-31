import org.tribuo.Dataset;
import org.tribuo.Model;
import org.tribuo.MutableDataset;
import org.tribuo.Trainer;
import org.tribuo.data.csv.CSVLoader;
import org.tribuo.evaluation.TrainTestSplitter;
import org.tribuo.math.optimisers.SGD;
import org.tribuo.regression.RegressionFactory;
import org.tribuo.regression.Regressor;
import org.tribuo.regression.evaluation.RegressionEvaluator;
import org.tribuo.regression.liblinear.LibLinearRegressionTrainer;
import org.tribuo.regression.liblinear.LinearRegressionType;
import org.tribuo.regression.liblinear.LinearRegressionType.LinearType;
import org.tribuo.regression.rtree.CARTRegressionTrainer;
import org.tribuo.regression.sgd.linear.LinearSGDTrainer;
import org.tribuo.regression.sgd.objectives.SquaredLoss;
import org.tribuo.regression.xgboost.XGBoostRegressionTrainer;
import org.tribuo.util.Util;

import java.nio.file.Paths;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;

public class Regression {

    private static String DATASET_PATH = "/home/dipanzan/IdeaProjects/machine-learning-java/dataset/";

    private static class DataPartition {
        private Dataset<Regressor> train, test;

        public DataPartition(Dataset<Regressor> train, Dataset<Regressor> test) {
            this.train = train;
            this.test = test;
        }
    }

    public static Model<Regressor> train(String name, Trainer<Regressor> trainer, Dataset<Regressor> trainData) {
        // Train the model
        var startTime = System.currentTimeMillis();
        Model<Regressor> model = trainer.train(trainData);
        var endTime = System.currentTimeMillis();
        System.out.println("Training " + name + " took " + Util.formatDuration(startTime, endTime));
        // Evaluate the model on the training data
        // This is a useful debugging tool to check the model actually learned something
        RegressionEvaluator eval = new RegressionEvaluator();
        var evaluation = eval.evaluate(model, trainData);
        // We create a dimension here to aid pulling out the appropriate statistics.
        // You can also produce the String directly by calling "evaluation.toString()"
        var dimension = new Regressor("DIM-0", Double.NaN);
        // Don't report training scores
        //System.out.printf("Evaluation (train):%n  RMSE %f%n  MAE %f%n  R^2 %f%n",
        //        evaluation.rmse(dimension), evaluation.mae(dimension), evaluation.r2(dimension));
        return model;
    }

    public static Object evaluate(Model<Regressor> model, Dataset<Regressor> testData) {
        // Evaluate the model on the test data
        RegressionEvaluator eval = new RegressionEvaluator();
        var evaluation = eval.evaluate(model, testData);
        // We create a dimension here to aid pulling out the appropriate statistics.
        // You can also produce the String directly by calling "evaluation.toString()"
        var dimension = new Regressor("DIM-0", Double.NaN);
        System.out.printf("Evaluation (test):%n  RMSE: %f%n  MAE:  %f%n  R^2:  %f%n",
                evaluation.rmse(dimension), evaluation.mae(dimension), evaluation.r2(dimension));

        return 0;
    }

    public static DataPartition loadDataset() throws Exception {
        var regressionFactory = new RegressionFactory();
        var csvLoader = new CSVLoader<>(regressionFactory);
        var startTime = System.currentTimeMillis();
        var carsSource = csvLoader.loadDataSource(Paths.get(DATASET_PATH + "cleanedCars.csv"), "price_usd");
        var endTime = System.currentTimeMillis();
        System.out.println("Loading took " + Util.formatDuration(startTime, endTime));
        startTime = System.currentTimeMillis();
        var splitter = new TrainTestSplitter<>(carsSource, 0.8f, 0L);
        endTime = System.currentTimeMillis();
        System.out.println("Splitting took " + Util.formatDuration(startTime, endTime));

        Dataset<Regressor> trainData = new MutableDataset<>(splitter.getTrain());
        Dataset<Regressor> evalData = new MutableDataset<>(splitter.getTest());

        System.out.println(String.format("Training data size = %d, number of features = %d", trainData.size(), trainData.getFeatureMap().size()));
        System.out.println(String.format("Testing data size = %d, number of features = %d", evalData.size(), evalData.getFeatureMap().size()));

        DataPartition dp = new DataPartition(trainData, evalData);

        return dp;
    }

    public static void carRegression(DataPartition dp, int epochs) throws Exception {
        List<Trainer> trainers = List.of(
                new LinearSGDTrainer(
                        new SquaredLoss(),           // loss function
                        SGD.getLinearDecaySGD(0.01), // gradient descent algorithm
                        epochs,                           // number of training epochs
                        dp.train.size() / 4,          // logging interval
                        1,                           // minibatch size
                        1L                           // RNG seed
                ),
                new LibLinearRegressionTrainer(
                        new LinearRegressionType(LinearType.L2R_L2LOSS_SVR),
                        1.0,    // cost penalty
                        epochs,   // max iterations
                        0.1,    // termination criteria
                        0.1     // epsilon
                ),
                new CARTRegressionTrainer(10),
                new XGBoostRegressionTrainer(75)
        );

        List<Model> models = trainers.stream()
                .map(t -> (Model) Method.execute(() -> train(t.toString(), t, dp.train), t.getClass().getSimpleName() + ".train()"))
                .collect(Collectors.toList());

        List<Object> evaluations = models.stream()
                .map(m -> Method.execute(() -> evaluate(m, dp.test), m.getClass().getSimpleName() + ".evaluate()"))
                .collect(Collectors.toList());
    }

    public static void main(String[] args) throws Exception {
        var logger = Logger.getLogger(org.tribuo.common.sgd.AbstractSGDTrainer.class.getName());
        logger.setLevel(Level.OFF);

        int epochs = Integer.parseInt(args[0]);
        carRegression(loadDataset(), epochs);
    }
}
