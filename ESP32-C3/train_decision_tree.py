import pandas as pd
import matplotlib.pyplot as plt

from sklearn.model_selection import train_test_split
from sklearn.tree import DecisionTreeClassifier, plot_tree
from sklearn.metrics import (
    accuracy_score,
    confusion_matrix,
    ConfusionMatrixDisplay
)


# ============================================================
# 기본 설정
# ============================================================

CSV_FILE = "fan_dataset.csv"


# ------------------------------------------------------------
# 사용할 가속도 축 선택
#
# "Z"   : Voltage, Current + Z축
# "XZ"  : Voltage, Current + X축 + Z축
# "XYZ" : Voltage, Current + X축 + Y축 + Z축
# ------------------------------------------------------------

AXIS_MODE = "Z"


# ============================================================
# 데이터 읽기
# ============================================================

df = pd.read_csv(CSV_FILE)

print("Dataset shape:", df.shape)
print()
print(df.head())
print()


# ============================================================
# Feature 선택
# ============================================================

if AXIS_MODE == "Z":

    FEATURES = [
        "Voltage",
        "Current",
        "Z_Freq",
        "Z_Energy"
    ]


elif AXIS_MODE == "XZ":

    FEATURES = [
        "Voltage",
        "Current",

        "X_Freq",
        "X_Energy",

        "Z_Freq",
        "Z_Energy"
    ]


elif AXIS_MODE == "XYZ":

    FEATURES = [
        "Voltage",
        "Current",

        "X_Freq",
        "X_Energy",

        "Y_Freq",
        "Y_Energy",

        "Z_Freq",
        "Z_Energy"
    ]


else:

    raise ValueError(
        'AXIS_MODE는 "Z", "XZ", "XYZ" 중 하나여야 합니다.'
    )


print("========================================")
print("Axis Mode :", AXIS_MODE)
print("Features  :", FEATURES)
print("========================================")
print()


# ============================================================
# 입력 Feature / Label 분리
# ============================================================

X = df[FEATURES]
y = df["Label"]


# ============================================================
# Train / Test 데이터 분리
# ============================================================

X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42,
    stratify=y
)


print("Training samples :", len(X_train))
print("Test samples     :", len(X_test))
print()


# ============================================================
# Decision Tree 생성
# ============================================================

model = DecisionTreeClassifier(
    max_depth=3,
    random_state=42
)


# ============================================================
# 모델 학습
# ============================================================

model.fit(
    X_train,
    y_train
)


# ============================================================
# Test 데이터 예측
# ============================================================

y_pred = model.predict(
    X_test
)


# ============================================================
# Accuracy
# ============================================================

accuracy = accuracy_score(
    y_test,
    y_pred
)


print("========================================")
print("Model Result")
print("========================================")

print(f"Axis Mode : {AXIS_MODE}")
print(f"Accuracy  : {accuracy:.4f}")
print(f"Accuracy  : {accuracy * 100:.2f} %")

print()


# ============================================================
# Tree 정보
# ============================================================

print("Tree depth :", model.get_depth())
print("Leaf nodes :", model.get_n_leaves())

print()


# ============================================================
# Feature Importance
# ============================================================

print("========================================")
print("Feature Importance")
print("========================================")

for name, importance in zip(
    FEATURES,
    model.feature_importances_
):

    print(
        f"{name:10s} : {importance:.4f}"
    )

print()


# ============================================================
# Confusion Matrix
# ============================================================

labels = [
    "NORMAL",
    "CONTACT",
    "AIRFLOW_RESTRICTED"
]


cm = confusion_matrix(
    y_test,
    y_pred,
    labels=labels
)


print("========================================")
print("Confusion Matrix")
print("========================================")

print(cm)
print()


# ============================================================
# 오분류 개수
# ============================================================

wrong_count = (y_test != y_pred).sum()

print(
    "Misclassified samples :",
    wrong_count
)

print()


# ============================================================
# Confusion Matrix 그래프
# ============================================================

disp = ConfusionMatrixDisplay(
    confusion_matrix=cm,
    display_labels=labels
)

disp.plot()

plt.title(
    f"Confusion Matrix - {AXIS_MODE}"
)

plt.tight_layout()

plt.show()


# ============================================================
# Decision Tree 그래프
# ============================================================

plt.figure(
    figsize=(16, 8)
)


plot_tree(
    model,
    feature_names=FEATURES,
    class_names=model.classes_,
    filled=True,
    rounded=True,
    fontsize=10
)


plt.title(
    f"Decision Tree - Feature Mode: {AXIS_MODE}"
)

plt.tight_layout()

plt.show()

print("\nFeature Importance")

for name, importance in zip(
    FEATURES,
    model.feature_importances_
):
    print(f"{name:10s} : {importance:.4f}")
