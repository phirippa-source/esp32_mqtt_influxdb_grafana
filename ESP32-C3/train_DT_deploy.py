import pandas as pd

from sklearn.model_selection import train_test_split
from sklearn.tree import DecisionTreeClassifier
from sklearn.metrics import accuracy_score, confusion_matrix


# ============================================================
# 기본 설정
# ============================================================

CSV_FILE = "fan_dataset.csv"

OUTPUT_HEADER_FILE = "fan_model.h"


# ------------------------------------------------------------
# 사용할 가속도 축 선택
#
# "Z"   : Voltage, Current + Z축
# "YZ"  : Voltage, Current + Y축 + Z축
# "XYZ" : Voltage, Current + X축 + Y축 + Z축
# ------------------------------------------------------------

AXIS_MODE = "Y"


# ============================================================
# 데이터 읽기
# ============================================================

df = pd.read_csv(CSV_FILE)

print("Dataset shape:", df.shape)
print()


# ============================================================
# Feature 선택
# ============================================================

if AXIS_MODE == "Y":

    FEATURES = [
        "Voltage",
        "Current",

        "Y_Freq",
        "Y_Energy"
    ]


elif AXIS_MODE == "YZ":

    FEATURES = [
        "Voltage",
        "Current",

        "Y_Freq",
        "Y_Energy",

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
        'AXIS_MODE는 "Y", "YZ", "XYZ" 중 하나여야 합니다.'
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
# 모델 성능 확인
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
# Decision Tree → C++ 코드 변환 함수
# ============================================================

def tree_to_cpp(
    model,
    feature_names
):

    tree = model.tree_

    cpp_lines = []


    # --------------------------------------------------------
    # Python Feature 이름을 C++ 변수 이름으로 변경
    # --------------------------------------------------------

    feature_variable_map = {

        "Voltage": "voltage",
        "Current": "current",

        "X_Freq": "xFreq",
        "X_Energy": "xEnergy",

        "Y_Freq": "yFreq",
        "Y_Energy": "yEnergy",

        "Z_Freq": "zFreq",
        "Z_Energy": "zEnergy"
    }


    # ========================================================
    # Tree를 재귀적으로 탐색
    # ========================================================

    def recurse(
        node,
        depth
    ):

        indent = "    " * depth


        # ----------------------------------------------------
        # Leaf Node
        # ----------------------------------------------------

        if tree.children_left[node] == tree.children_right[node]:

            class_index = tree.value[node][0].argmax()

            class_name = model.classes_[class_index]

            cpp_lines.append(
                f'{indent}return "{class_name}";'
            )

            return


        # ----------------------------------------------------
        # Decision Node
        # ----------------------------------------------------

        feature_index = tree.feature[node]

        feature_name = feature_names[feature_index]

        cpp_variable = feature_variable_map[
            feature_name
        ]

        threshold = tree.threshold[node]


        cpp_lines.append(
            f"{indent}if ({cpp_variable} <= {threshold:.6f}f) {{"
        )


        recurse(
            tree.children_left[node],
            depth + 1
        )


        cpp_lines.append(
            f"{indent}}} else {{"
        )


        recurse(
            tree.children_right[node],
            depth + 1
        )


        cpp_lines.append(
            f"{indent}}}"
        )


    # ========================================================
    # predictFanState() 함수 시작
    # ========================================================

    function_lines = [

        "inline const char* predictFanState(",

        "    float voltage,",
        "    float current,",

        "    float xFreq,",
        "    float xEnergy,",

        "    float yFreq,",
        "    float yEnergy,",

        "    float zFreq,",
        "    float zEnergy",

        ")",

        "{"
    ]


    recurse(
        0,
        1
    )


    function_lines.extend(
        cpp_lines
    )


    function_lines.append(
        "}"
    )


    return "\n".join(
        function_lines
    )


# ============================================================
# C++ Decision Tree 코드 생성
# ============================================================

cpp_model_code = tree_to_cpp(
    model,
    FEATURES
)


# ============================================================
# fan_model.h 파일 내용 생성
# ============================================================

header_code = f"""\
#ifndef FAN_MODEL_H
#define FAN_MODEL_H


// ============================================================
// 자동 생성된 Decision Tree 모델
//
// Source Dataset : {CSV_FILE}
// Axis Mode      : {AXIS_MODE}
// Tree Depth     : {model.get_depth()}
//
// 이 파일은 train_DT_deploy.py에 의해 자동 생성되었습니다.
// 직접 수정하지 않는 것을 권장합니다.
// ============================================================


{cpp_model_code}


#endif
"""


# ============================================================
# fan_model.h 저장
# ============================================================

with open(
    OUTPUT_HEADER_FILE,
    "w",
    encoding="utf-8"
) as file:

    file.write(
        header_code
    )


# ============================================================
# 결과 출력
# ============================================================

print("========================================")
print("Deployment")
print("========================================")

print(
    f"{OUTPUT_HEADER_FILE} 파일이 생성되었습니다."
)

print()

print(
    "Arduino 프로젝트 폴더에 "
    f"{OUTPUT_HEADER_FILE} 파일을 복사하세요."
)

print()

print(
    '#include "fan_model.h"'
)

print()

print(
    "를 Arduino 코드에 추가하면 됩니다."
)

print()


# ============================================================
# 생성된 C++ 모델 확인
# ============================================================

print("========================================")
print("Generated C++ Model")
print("========================================")

print()

print(
    cpp_model_code
)
