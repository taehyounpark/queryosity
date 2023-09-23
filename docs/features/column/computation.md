```mermaid
  graph BT
  A[column] --> X[definition];
  B[column] --> X;
  B --> Y[definition];
  C[constant] --> Y;
  X --> Z[definition];
  Y --> Z;
  B --> Z;
```