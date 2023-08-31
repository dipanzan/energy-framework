import java.nio.file.Paths;
import java.nio.file.Files;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;

import org.tribuo.*;
import org.tribuo.data.csv.CSVIterator;
import org.tribuo.evaluation.TrainTestSplitter;
import org.tribuo.data.csv.CSVLoader;
import org.tribuo.util.Util;
import org.tribuo.clustering.*;
import org.tribuo.clustering.evaluation.*;
import org.tribuo.clustering.kmeans.*;
import org.tribuo.clustering.kmeans.KMeansTrainer.Distance;
import org.tribuo.clustering.kmeans.KMeansTrainer.Initialisation;

import javax.xml.crypto.Data;

public class Clustering {
    private static String DATASET_PATH = "/home/dipanzan/IdeaProjects/machine-learning-java/dataset/";

    private static class DataPartition {
        private Dataset<ClusterID> train, test;

        public DataPartition(Dataset<ClusterID> train, Dataset<ClusterID> test) {
            this.train = train;
            this.test = test;
        }
    }

    public static DataPartition loadGaussianDataset() throws Exception {
        var clusteringFactory = new ClusteringFactory();
        var csvLoader = new CSVLoader<>(clusteringFactory);
        var startTime = System.currentTimeMillis();
        var gaussianSource = csvLoader.loadDataSource(Paths.get(DATASET_PATH + "gaussianBlobs.csv"), "Cluster");
        var endTime = System.currentTimeMillis();
        System.out.println("Loading took " + Util.formatDuration(startTime, endTime));
        startTime = System.currentTimeMillis();
        var splitter = new TrainTestSplitter<>(gaussianSource, 0.8f, 0L);
        endTime = System.currentTimeMillis();
        System.out.println("Splitting took " + Util.formatDuration(startTime, endTime));

        Dataset<ClusterID> trainData = new MutableDataset<>(splitter.getTrain());
        Dataset<ClusterID> evalData = new MutableDataset<>(splitter.getTest());

        System.out.println(String.format("Training data size = %d, number of features = %d", trainData.size(), trainData.getFeatureMap().size()));
        System.out.println(String.format("Testing data size = %d, number of features = %d", evalData.size(), evalData.getFeatureMap().size()));

        DataPartition dp = new DataPartition(trainData, evalData);

        return dp;
    }

    // Note: the types including generics were tricky to get working
    public static Model train(String name, Trainer trainer, Dataset<ClusterID> trainData) {
        // Train the model
        var startTime = System.currentTimeMillis();
        var model = trainer.train(trainData);
        var endTime = System.currentTimeMillis();
        System.out.println("Training " + name + " took " + Util.formatDuration(startTime, endTime));
        return model;
    }

    public static Object evaluate(Model model, Dataset<ClusterID> testData) {
        // Evaluate the model on the test data
        var eval = new ClusteringEvaluator();
        var evaluation = eval.evaluate(model, testData);
        System.out.println(evaluation.toString());
//        System.out.println(evaluation.getConfusionMatrix().toString());

        return 0;
    }

    public static void gaussianClustering(DataPartition dp, int epochs) {
        var trainers = List.of(
                new KMeansTrainer(6, epochs, Distance.EUCLIDEAN, 4, 1),
                new KMeansTrainer(6, epochs, Distance.EUCLIDEAN, Initialisation.PLUSPLUS, 4, 1)
        );

        List<Model> models = trainers.stream()
                .map(t -> (Model) Method.execute(() -> train(t.toString(), t, dp.train), t.getClass().getSimpleName() + ".train()"))
                .collect(Collectors.toList());

        List<Object> evaluations = models.stream()
                .map(m -> Method.execute(() -> evaluate(m, dp.test), m.getClass().getSimpleName() + ".evaluate()"))
                .collect(Collectors.toList());
    }

    public static void main(String[] args) throws Exception {
        var logger1 = Logger.getLogger(KMeansTrainer.class.getName());
        logger1.setLevel(Level.OFF);

        var logger2 = Logger.getLogger(CSVIterator.class.getName());
        logger2.setLevel(Level.OFF);

        int epochs = Integer.parseInt(args[0]);

        DataPartition dp = (DataPartition) Method.execute(() -> {
            try {
                return loadGaussianDataset();
            } catch (Exception e) {
                return null;
            }
        }, "loadGaussianDataset()");
        gaussianClustering(dp, epochs);
    }
}
