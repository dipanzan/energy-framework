import numpy as np
import pandas as pd
from sklearn.datasets import make_blobs

X, y = make_blobs(n_samples=6000000, centers=6, n_features=5, random_state=0)
print(X.shape)

print(X[0])

df = pd.DataFrame({'Feature1': X[:, 0], 'Feature2': X[:, 1], 'Feature3': X[:, 2],
                   'Feature4': X[:, 3], 'Feature5': X[:, 4], 'Cluster': y})
df.head(5)

df.to_csv('gaussianBlobs.csv', index=False)
