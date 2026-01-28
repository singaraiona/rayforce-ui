# Rayfall Function Reference

## Special Forms

Only three true special forms:

| Form | Usage | Description |
|------|-------|-------------|
| `self` | `self` | Self-reference in recursive contexts |
| `if` | `(if cond then else)` | Conditional branching |
| `try` | `(try expr handler)` | Error handling with fallback |

## Core Functions

### Variable Binding

| Function | Signature | Description |
|----------|-----------|-------------|
| `set` | `(set name value)` | Global variable assignment |
| `let` | `(let name value)` | Local variable (function scope) |
| `fn` | `(fn [args] body)` | Lambda function definition |
| `do` | `(do expr1 expr2 ...)` | Sequential evaluation, returns last |
| `return` | `(return value)` | Early return from function |
| `exit` | `(exit code)` | Exit program with code |

### Error Handling

| Function | Signature | Description |
|----------|-----------|-------------|
| `raise` | `(raise message)` | Throw exception |
| `quote` | `(quote expr)` or `'expr` | Prevent evaluation |
| `eval` | `(eval expr)` | Evaluate expression/string |
| `parse` | `(parse string)` | Parse string to AST |

## Arithmetic Operators

All broadcast scalars to vectors.

| Op | Example | Result |
|----|---------|--------|
| `+` | `(+ 1 2)` | `3` |
| `+` | `(+ 10 [1 2 3])` | `[11 12 13]` |
| `-` | `(- 5 3)` | `2` |
| `*` | `(* 2 [1 2 3])` | `[2 4 6]` |
| `/` | `(/ 10 3)` | `3` (integer div) |
| `%` | `(% 10 3)` | `1` |
| `neg` | `(neg [1 -2 3])` | `[-1 2 -3]` |

### Math Functions

| Function | Description | Example |
|----------|-------------|---------|
| `sqrt` | Square root | `(sqrt 16)` → `4.0` |
| `pow` | Power | `(pow 2 10)` → `1024` |
| `round` | Round to nearest | `(round 3.7)` → `4` |
| `floor` | Round down | `(floor 3.7)` → `3` |
| `ceil` | Round up | `(ceil 3.2)` → `4` |

## Comparison Operators

Return boolean or boolean vector.

| Op | Description | Example |
|----|-------------|---------|
| `==` | Equal | `(== 1 1)` → `true` |
| `!=` | Not equal | `(!= 1 2)` → `true` |
| `>` | Greater than | `(> [1 2 3] 2)` → `[false false true]` |
| `>=` | Greater or equal | `(>= 2 2)` → `true` |
| `<` | Less than | `(< 1 2)` → `true` |
| `<=` | Less or equal | `(<= [1 2 3] 2)` → `[true true false]` |

## Logical Operators

| Op | Description | Example |
|----|-------------|---------|
| `and` | Logical AND | `(and true false)` → `false` |
| `or` | Logical OR | `(or true false)` → `true` |
| `not` | Logical NOT | `(not true)` → `false` |

## Aggregation Functions

Operate on vectors, return scalar.

| Function | Description | Example |
|----------|-------------|---------|
| `count` | Element count | `(count [1 2 3])` → `3` |
| `sum` | Total | `(sum [1 2 3])` → `6` |
| `avg` | Mean | `(avg [1 2 3])` → `2.0` |
| `med` | Median | `(med [1 3 2])` → `2` |
| `min` | Minimum | `(min [3 1 2])` → `1` |
| `max` | Maximum | `(max [3 1 2])` → `3` |
| `dev` | Std deviation | `(dev [1 2 3])` |
| `first` | First element | `(first [1 2 3])` → `1` |
| `last` | Last element | `(last [1 2 3])` → `3` |

## Vector Operations

### Construction

| Function | Description | Example |
|----------|-------------|---------|
| `til` | Range 0 to n-1 | `(til 5)` → `[0 1 2 3 4]` |
| `enlist` | Create vector | `(enlist 1 2 3)` → `[1 2 3]` |
| `list` | Heterogeneous list | `(list 1 'a "b")` |
| `concat` | Concatenate | `(concat [1 2] [3 4])` → `[1 2 3 4]` |

### Access

| Function | Description | Example |
|----------|-------------|---------|
| `at` | Index/key access | `(at [1 2 3] 1)` → `2` |
| `at` | Column access | `(at table 'col)` |
| `take` | First/last n | `(take [1 2 3 4] 2)` → `[1 2]` |
| `take` | Last n (negative) | `(take [1 2 3 4] -2)` → `[3 4]` |

### Transformation

| Function | Description | Example |
|----------|-------------|---------|
| `reverse` | Reverse order | `(reverse [1 2 3])` → `[3 2 1]` |
| `distinct` | Unique values | `(distinct [1 1 2])` → `[1 2]` |
| `raze` | Flatten nested | `(raze [[1 2] [3 4]])` → `[1 2 3 4]` |

## Set Operations

| Function | Description | Example |
|----------|-------------|---------|
| `union` | Set union | `(union [1 2] [2 3])` → `[1 2 3]` |
| `sect` | Intersection | `(sect [1 2 3] [2 3 4])` → `[2 3]` |
| `except` | Difference | `(except [1 2 3] [2])` → `[1 3]` |
| `in` | Membership test | `(in 2 [1 2 3])` → `true` |
| `find` | Find index | `(find [1 2 3] 2)` → `1` |
| `within` | Range check | `(within [5] [1 10])` → `[true]` |

## Sorting Functions

| Function | Description | Example |
|----------|-------------|---------|
| `asc` | Sort ascending | `(asc [3 1 2])` → `[1 2 3]` |
| `desc` | Sort descending | `(desc [3 1 2])` → `[3 2 1]` |
| `iasc` | Indices for asc | `(iasc [3 1 2])` → `[1 2 0]` |
| `idesc` | Indices for desc | `(idesc [3 1 2])` → `[0 2 1]` |
| `rank` | Rank of each | `(rank [30 10 20])` → `[2 0 1]` |
| `xasc` | Sort table asc | `(xasc [col] table)` |
| `xdesc` | Sort table desc | `(xdesc [col] table)` |
| `xrank` | Bucket rank | `(xrank [1 2 3 4] 2)` → `[0 0 1 1]` |
| `xbar` | Round to bucket | `(xbar 5 [3 7 12])` → `[0 5 10]` |

## Higher-Order Functions

| Function | Description | Example |
|----------|-------------|---------|
| `map` | Apply to each | `(map (fn [x] (* x 2)) [1 2 3])` → `[2 4 6]` |
| `pmap` | Parallel map | `(pmap fn vec)` |
| `filter` | Select by mask | `(filter [1 2 3] [true false true])` → `[1 3]` |
| `fold` | Reduce left | `(fold + [1 2 3])` → `6` |
| `scan` | Running fold | `(scan + [1 2 3])` → `[1 3 6]` |
| `apply` | Apply element-wise | `(apply + [1 2] [3 4])` → `[4 6]` |

## Dictionary Functions

| Function | Description | Example |
|----------|-------------|---------|
| `dict` | Create dict | `(dict [a b] [1 2])` → `{a: 1 b: 2}` |
| `key` | Get keys | `(key {a: 1 b: 2})` → `[a b]` |
| `value` | Get values | `(value {a: 1 b: 2})` → `[1 2]` |

## Table Functions

| Function | Description | Example |
|----------|-------------|---------|
| `table` | Create table | `(table [id name] (list [1 2] ['A 'B]))` |
| `row` | Row count | `(row table)` |

## Type Functions

| Function | Description | Example |
|----------|-------------|---------|
| `type` | Get type | `(type [1 2 3])` → `I64` |
| `as` | Type cast | `(as 'F64 [1 2])` → `[1.0 2.0]` |
| `nil?` | Is null? | `(nil? null)` → `true` |

## String Functions

| Function | Description | Example |
|----------|-------------|---------|
| `like` | Pattern match | `(like "hello" "he*")` → `true` |
| `format` | String format | `(format "%/%/%" y m d)` |

## I/O Functions

| Function | Description | Example |
|----------|-------------|---------|
| `read` | Read file | `(read "/path/file")` |
| `write` | Write file | `(write "/path" data)` |
| `read-csv` | Read CSV | `(read-csv [I64 SYM F64] "f.csv")` |
| `write-csv` | Write CSV | `(write-csv "f.csv" table)` |
| `load` | Load and eval | `(load "script.rfl")` |
| `print` | Print | `(print "text")` |
| `println` | Print with newline | `(println "text")` |

## Time Functions

| Function | Description | Example |
|----------|-------------|---------|
| `date` | Get date | `(date 'local)` or `(date 'global)` |
| `time` | Get time | `(time 'local)` |
| `timestamp` | Get timestamp | `(timestamp 'global)` |

## Environment Functions

| Function | Description | Example |
|----------|-------------|---------|
| `env` | All variables | `(env)` |
| `resolve` | Get variable | `(resolve 'x)` |
| `args` | CLI arguments | `(args)` |

## Persistence Functions

| Function | Description |
|----------|-------------|
| `set-splayed` | Save splayed table |
| `get-splayed` | Load splayed table |
| `set-parted` | Save partitioned table |
| `get-parted` | Load partitioned table |

## Query Functions

See `queries.md` for detailed query documentation.

| Function | Description |
|----------|-------------|
| `select` | Query with filter/aggregate |
| `insert` | Add rows to table |
| `update` | Modify existing rows |
| `upsert` | Insert or update by key |
| `alter` | Modify vector/column |
| `left-join` | Left outer join |
| `inner-join` | Inner join |
| `asof-join` | Time-series asof join |
| `window-join` | Window aggregation join |
