---
name: rayfall
description: Rayfall language reference for RayforceDB. This skill provides syntax, types, functions, and examples for the Rayfall query language. Use when writing or debugging Rayfall code (.rfl files), understanding Rayfall syntax, creating queries for RayLens dashboards, or working with RayforceDB.
---

# Rayfall Language Reference

## Overview

**Rayfall** is a pure LISP-like language for the RayforceDB columnar database engine. It features:
- Simple LISP syntax with minimal parentheses
- Vectorized operations for columnar data
- First-class tables, vectors, and aggregations
- Zero-copy data binding between queries and UI

**File Extension**: `.rfl`

## Special Forms vs Functions

Only three true special forms exist:
- `self` - Self reference
- `if` - Conditional branching
- `try` - Error handling

**Everything else is a function**, including `fn`, `do`, `set`, `let`, `select`, etc.

## Data Types

### Scalar Types

| Type | Code | Format | Examples |
|------|------|--------|----------|
| Boolean | `-1` | `true`/`false` | `true`, `false` |
| Byte | `-2` | `0xNN` | `0x1a`, `0xff` |
| Short (I16) | `-3` | `Ni` | `32i`, `-100i` |
| Int (I32) | `-4` | `Nl` | `1000l` |
| Long (I64) | `-5` | `N` | `42`, `-999` |
| Symbol | `-6` | `'sym` | `'AAPL`, `'name` |
| Date | `-7` | `YYYY.MM.DD` | `2025.01.15` |
| Time | `-8` | `HH:MM:SS.mmm` | `09:30:00` |
| Timestamp | `-9` | `YYYY.MM.DDTHH:MM:SS` | `2025.01.15T09:30:00` |
| Char | `-10` | `'c'` | `'a'`, `'Z'` |
| Float (F64) | `-11` | `N.N` | `3.14`, `0.99` |
| String | `-12` | `"text"` | `"hello"` |

### Null Values

```clj
null    ;; Generic null
0Ni     ;; Null I32
0Nl     ;; Null I64
0Nf     ;; Null F64
0W      ;; Positive infinity
-0W     ;; Negative infinity
```

### Composite Types

```clj
;; Vector - homogeneous, square brackets
[1 2 3 4 5]
['AAPL 'GOOG 'MSFT]

;; List - heterogeneous
(list 1 'a "text" 2025.01.01)

;; Dictionary
(dict [a b c] [1 2 3])
{a: 1 b: 2 c: 3}

;; Table - columns are symbols, values are vectors
(table [id name age] (list [1 2 3] ['Alice 'Bob 'Charlie] [25 30 35]))
```

## Syntax

### Function Calls

```clj
(function arg1 arg2)
(+ 1 2 3)
(select {from: table where: condition})
```

### Comments

```clj
;; Line comment
(+ 1 2) ;; Inline comment
```

### Dictionary Literals

```clj
{key: value key2: value2}
{name: "Alice" age: 25 dept: 'IT}
```

## Control Flow

### Conditionals

```clj
(if condition then-expr else-expr)
(if (> x 10) "big" "small")
(if (nil? x) "none" x)
```

### Sequential Execution

```clj
(do
  (set x 10)
  (set y 20)
  (+ x y))  ;; Returns 30
```

### Error Handling

```clj
(try (/ 10 0) (fn [e] 0))       ;; Returns 0 on error
(try risky-expr fallback-val)
(raise "Error message")
```

### Early Return

```clj
(fn [x]
  (if (< x 0) (return "negative"))
  (* x x))
```

## Variable Binding

```clj
;; Global assignment
(set x 100)
(set employees (table ...))

;; Local variable (function scope)
(fn [x]
  (let y (+ x 10))
  y)
```

## Function Definition

```clj
;; Lambda
(fn [x y] (+ x (* y 2)))

;; Named function
(set square (fn [x] (* x x)))
(square 5)  ;; 25

;; With locals
(set calc (fn [price qty rate]
  (let subtotal (* price qty))
  (let tax (* subtotal rate))
  (+ subtotal tax)))
```

## Operators

### Arithmetic (broadcast to vectors)

```clj
(+ a b)    ;; Add
(- a b)    ;; Subtract
(* a b)    ;; Multiply
(/ a b)    ;; Divide (0 → null)
(% a b)    ;; Modulo

;; Broadcasting
(+ 10 [1 2 3])        ;; [11 12 13]
(* [1 2 3] [4 5 6])   ;; [4 10 18]
```

### Comparison

```clj
(== a b)   ;; Equal
(!= a b)   ;; Not equal
(> a b)    ;; Greater
(>= a b)   ;; Greater or equal
(< a b)    ;; Less
(<= a b)   ;; Less or equal
```

### Logical

```clj
(and x y)
(or x y)
(not x)
```

### Pattern Matching

```clj
(like "hello" "he*")   ;; Wildcard: * ? [chars]
(like "test" "t?st")   ;; Single char match
```

## Aggregation Functions

```clj
(sum [1 2 3])      ;; 6
(avg [1 2 3])      ;; 2.0
(min [3 1 2])      ;; 1
(max [3 1 2])      ;; 3
(med [1 3 2])      ;; 2
(dev [1 2 3])      ;; Standard deviation
(count x)          ;; Element count
(first [1 2 3])    ;; 1
(last [1 2 3])     ;; 3
```

## Vector Operations

```clj
(at [1 2 3] 1)         ;; 2 (index access)
(at table 'column)     ;; Column access
(take [1 2 3 4 5] 3)   ;; [1 2 3]
(take [1 2 3 4 5] -2)  ;; [4 5]
(til 5)                ;; [0 1 2 3 4]
(enlist 1 2 3)         ;; [1 2 3]
(concat [1 2] [3 4])   ;; [1 2 3 4]
(reverse [1 2 3])      ;; [3 2 1]
(distinct [1 1 2 2])   ;; [1 2]
```

## Set Operations

```clj
(union [1 2] [2 3])      ;; [1 2 3]
(sect [1 2 3] [2 3 4])   ;; [2 3] (intersection)
(except [1 2 3] [2])     ;; [1 3]
(in 2 [1 2 3])           ;; true
(find [1 2 3] 2)         ;; 1 (index)
(within [5] [1 10])      ;; [true]
```

## Higher-Order Functions

```clj
(map (fn [x] (* x 2)) [1 2 3])   ;; [2 4 6]
(pmap (fn [x] (* x 2)) [1 2 3])  ;; Parallel map
(filter [1 2 3] [true false true])  ;; [1 3]
(fold max [3 1 4 1 5])           ;; 5
(apply + [1 2 3] [4 5 6])        ;; [5 7 9]
```

## Sorting

```clj
(asc [3 1 2])       ;; [1 2 3]
(desc [3 1 2])      ;; [3 2 1]
(iasc [3 1 2])      ;; [1 2 0] (sort indices)
(idesc [3 1 2])     ;; [0 2 1]
(rank [30 10 20])   ;; [2 0 1]
(xasc [col] table)  ;; Sort table by column
(xdesc [col] table)
```

## Type Operations

```clj
(type [1 2 3])       ;; I64
(as 'F64 [1 2 3])    ;; [1.0 2.0 3.0]
(nil? null)          ;; true
```

## Query Operations

### SELECT

```clj
;; Basic select
(select {
  name: name
  salary: salary
  from: employees
  where: (> salary 70000)
})

;; Aggregation with group-by
(select {
  dept: dept
  avg_salary: (avg salary)
  headcount: (count name)
  from: employees
  by: dept
})

;; Multi-column grouping
(select {
  region: region
  total: (sum amount)
  from: sales
  by: {region: region year: year}
})
```

### INSERT

```clj
;; Single row
(insert employees (list 'Charlie 35))

;; Multiple rows
(insert employees (list ['David 'Eve] [40 25]))

;; In-place with quoted symbol
(insert 'employees (list "Mike" 75))
```

### UPDATE

```clj
(update {
  salary: (* salary 1.1)
  from: employees
  where: (> salary 70000)
})

(update {
  bonus: 1000
  from: employees
  by: dept
  where: (== rating 'A)
})
```

### UPSERT

```clj
;; key_col_index, new_data
(upsert table 0 (list [1 2] ['Alice 'Bob]))
(upsert 'table 0 (list id name))
```

### ALTER

```clj
;; Modify vector element
(alter prices + 1 10)        ;; Add 10 at index 1

;; Modify table column
(alter trades + 'price 10)   ;; Add 10 to price column
```

## Joins

```clj
;; Left join - keep all left rows
(left-join [order_id] trades orders)

;; Inner join - matching rows only
(inner-join [order_id] trades orders)

;; Asof join - time-series (greatest ≤)
(asof-join [Sym Ts] trades quotes)

;; Window join - aggregate within time window
(set intervals (map-left + [-1000 1000] (at trades 'Ts)))
(window-join [Sym Ts] intervals trades quotes {
  bid: (min Bid)
  ask: (max Ask)
})
```

## I/O

```clj
(read "/path/to/file")
(write "/path" data)
(read-csv [I64 SYMBOL F64] "/path.csv")
(write-csv "/path.csv" table)
(load "file.rfl")
```

## Time Functions

```clj
(date 'local)        ;; Local date
(date 'global)       ;; UTC date
(time 'local)        ;; Local time
(timestamp 'global)  ;; UTC timestamp
```

## Persistence

### Splayed Tables

```clj
;; Save (each column = separate file)
(set-splayed "/tmp/db/tab/" table)
(set-splayed "/tmp/db/tab/" table "/tmp/db/sym")

;; Load
(get-splayed "/tmp/db/tab/" "/tmp/db/tab.sym")
```

### Parted Tables

```clj
;; Save partitioned
(set-parted "/tmp/db/" table)

;; Load
(get-parted "/tmp/db/" 'tablename)
```

## Examples

### Market Data Query

```clj
(set trades (table [sym ts price size] (list
  ['AAPL 'GOOG 'AAPL 'MSFT]
  [09:30:00 09:30:01 09:30:02 09:30:03]
  [150.25 2800.50 150.30 310.00]
  [100 50 200 150])))

;; VWAP by symbol
(select {
  sym: sym
  vwap: (/ (sum (* price size)) (sum size))
  volume: (sum size)
  from: trades
  by: sym
})
```

### Data Pipeline

```clj
(set raw (read-csv [SYMBOL TIMESTAMP F64 I64] "trades.csv"))
(set cleaned (select {
  sym: sym
  ts: ts
  price: price
  size: size
  from: raw
  where: (and (> price 0) (> size 0))
}))
(set enriched (left-join [sym] cleaned instruments))
(write-csv "output.csv" enriched)
```

### Custom Aggregation

```clj
(set range-calc (fn [prices]
  (- (max prices) (min prices))))

(select {
  sym: sym
  price_range: (range-calc price)
  from: trades
  by: sym
})
```

## Key Differences from Standard LISP

1. **Vectorized**: Arithmetic broadcasts element-wise
2. **Tables**: First-class relational type with symbol columns
3. **Dict Args**: `{key: value}` syntax for readability
4. **Immutable Default**: `set` reassigns; quote for in-place ops
5. **Zero-Copy**: References to columnar data
6. **Columnar Thinking**: Operations work column-wise

## Related Files

- `references/types.md` - Detailed type system documentation
- `references/functions.md` - Complete function reference
- `references/queries.md` - Query operation details
