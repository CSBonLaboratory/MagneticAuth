import "math"


attemptCumulativeTable = from(bucket: "MagneticAttempts")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "magnetic" and r["id"] == "3")
  |> aggregateWindow(every: 100ms, fn: sum, createEmpty: false)
  |> drop(columns: ["_start", "_stop", "_time", "_field", "id", "_measurement"])
  |> cumulativeSum()
  |> map(fn: (r) => ({r with rowId: 1}))
  |> cumulativeSum(columns: ["rowId"])
  |> rename(columns: {_value: "AttemptSum"})

signatureCumulativeTable = from(bucket: "Signatures")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "magnetic")
  |> filter(fn: (r) => r["id"] == "3")
  |> aggregateWindow(every: 100ms, fn: sum, createEmpty: false)
  |> drop(columns: ["_start", "_stop", "_time", "_field", "id", "_measurement"])
  |> cumulativeSum()
  |> map(fn: (r) => ({r with rowId: 1}))
  |> cumulativeSum(columns: ["rowId"])
  |> rename(columns: {_value: "SignSum"})


SumsTable = join(tables: {Tc1: signatureCumulativeTable, Tc2: attemptCumulativeTable}, on: ["rowId"])
              |> map(fn: (r) => ({r with SignSumSquare : r.SignSum * r.SignSum}))
              |> map(fn: (r) => ({r with AttemptSumSquare : r.AttemptSum * r.AttemptSum}))
              |> map(fn: (r) => ({r with SignAttemptProduct : r.SignSum * r.AttemptSum}))


SignSumSum = SumsTable |> sum(column: "SignSum") |> set(key: "PearsonRequirement", value: "yes") |> rename(columns: {SignSum: "S(X)"})
AttemptSumSum = SumsTable |> sum(column: "AttemptSum") |> set(key: "PearsonRequirement", value: "yes") |> rename(columns: {AttemptSum: "S(Y)"})
SignSumSquareSum = SumsTable |> sum(column: "SignSumSquare") |> set(key: "PearsonRequirement", value: "yes") |> rename(columns: {SignSumSquare: "S(X2)"})
AttemptSumSquareSum = SumsTable |> sum(column: "AttemptSumSquare") |> set(key: "PearsonRequirement", value: "yes") |> rename(columns: {AttemptSumSquare: "S(Y2)"})
SignAttemptProductSum = SumsTable |> sum(column: "SignAttemptProduct") |> set(key: "PearsonRequirement", value: "yes") |> rename(columns: {SignAttemptProduct: "S(XY)"})
CountTable = SumsTable |> count(column: "rowId") |> set(key: "PearsonRequirement", value: "yes") |> rename(columns: {rowId: "n"})

FirstJoin  = join(tables: {TC1: SignSumSum, TC2: AttemptSumSum}, on: ["PearsonRequirement"])
SecondJoin = join(tables: {TC1: FirstJoin, TC2: SignSumSquareSum}, on: ["PearsonRequirement"])
ThirdJoin  = join(tables: {TC1: SecondJoin, TC2: AttemptSumSquareSum}, on: ["PearsonRequirement"])
FourthJoin = join(tables: {TC1 : ThirdJoin, TC2: SignAttemptProductSum}, on: ["PearsonRequirement"])

AllReqsJoin = join(tables: {TC1: FourthJoin, TC2: CountTable}, on: ["PearsonRequirement"])
//               |> map(fn: (r) => (
//                 {r with Pearson: 
//                   (
//                     float(v : (r["n"] * r["S(XY)"] - r["S(X)"] * r["S(Y)"])) / 
//                     float(v : math.sqrt(x:
//                       (r["n"] * r["S(X2)"] - r["SX"] * r["SX"]) * (r["n"] * r["S(Y2)"] - r["SY"] * r["SY"])
//                     ))
//               )}))
AllReqsJoin