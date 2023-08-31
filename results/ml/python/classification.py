import numpy as np
import pandas as pd
from sklearn import preprocessing
from sklearn.linear_model import LogisticRegression, SGDClassifier
from sklearn.metrics import classification_report, confusion_matrix
# from sklearn import cross_validation
from sklearn.model_selection import train_test_split
from sklearn.neural_network import MLPClassifier
from sklearn.tree import DecisionTreeClassifier

from method import execute

dataset_path = '/home/dipanzan/IdeaProjects/energy-framework/results/ml/dataset/'


def iris_classifier():
    df = pd.read_csv(dataset_path + 'bezdekIris.data', header=None)
    X = np.array(df[[0, 1, 2, 3]])
    y = np.array(df[4])

    # class_le = LabelEncoder()
    # y = class_le.fit_transform(df[4].values)

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.3, random_state=1)
    print('Training data size = %d, number of features = %d' % (len(X_train), 4))
    print('Testing data size = %d, number of features = %d' % (len(X_test), 4))

    stdscaler = preprocessing.StandardScaler().fit(X_train)
    X_train_scaled = stdscaler.transform(X_train)
    X_test_scaled = stdscaler.transform(X_test)
    sgd = SGDClassifier(max_iter=10000, learning_rate='adaptive', epsilon=0.1, eta0=1.0)

    execute(lambda: sgd.fit(X_train, y_train), "SGDCClassifer.train()")

    predicted = execute(lambda: sgd.predict(X_test), "SGDCClassifer.predict()")
    print(classification_report(y_test, predicted))
    print(confusion_matrix(y_test, predicted, labels=['Iris-versicolor', 'Iris-virginica', 'Iris-setosa']))


def load_weather_data():
    df = pd.read_csv(dataset_path + 'cleanedWeatherAUS.csv', header=None)
    X = np.array(df[[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                     27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
                     51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61]])
    y = np.array(df[62])

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=1)
    print('Training data size = %d, number of features = %d' % (len(X_train), len(df.columns) - 1))
    print('Testing data size = %d, number of features = %d' % (len(X_test), len(df.columns) - 1))

    return X_train, X_test, y_train, y_test


def weather_classifier():
    X_train, X_test, y_train, y_test = execute(lambda: load_weather_data(), "weather_data.train_test_split()")

    sgd = SGDClassifier(max_iter=100, epsilon=0.1)
    lr = LogisticRegression(tol=0.1, solver='liblinear')
    cart = DecisionTreeClassifier()
    mlp = MLPClassifier(max_iter=100)

    execute(lambda: sgd.fit(X_train, y_train), "SGDCClassifer.fit()")
    execute(lambda: lr.fit(X_train, y_train), "LogisticRegression.fit()")
    execute(lambda: cart.fit(X_train, y_train), "DecisionTreeClassifier.fit()")
    execute(lambda: mlp.fit(X_train, y_train), "MLPClassifier.fit()")

    predicted = execute(lambda: sgd.predict(X_test), "SGDCClassifer.predict()")
    print(classification_report(y_test, predicted))

    predicted = execute(lambda: lr.predict(X_test), "LogisticRegression.predict()")
    print(classification_report(y_test, predicted))

    predicted = execute(lambda: cart.predict(X_test), "DecisionTreeClassifier.predict()")
    print(classification_report(y_test, predicted))

    predicted = execute(lambda: mlp.predict(X_test), "MLPClassifier.predict()")
    print(classification_report(y_test, predicted))


def main():
    weather_classifier()


if __name__ == "__main__":
    main()
