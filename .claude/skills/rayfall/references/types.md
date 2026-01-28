# Rayfall Type System

## Type Codes

Rayfall uses numeric type codes. Negative codes are scalars, positive are vectors.

| Type | Scalar Code | Vector Code | Size |
|------|-------------|-------------|------|
| Boolean | `-1` | `1` | 1 byte |
| Byte (U8) | `-2` | `2` | 1 byte |
| Short (I16) | `-3` | `3` | 2 bytes |
| Int (I32) | `-4` | `4` | 4 bytes |
| Long (I64) | `-5` | `5` | 8 bytes |
| Symbol | `-6` | `6` | 8 bytes |
| Date | `-7` | `7` | 4 bytes |
| Time | `-8` | `8` | 4 bytes |
| Timestamp | `-9` | `9` | 8 bytes |
| Char (C8) | `-10` | `10` | 1 byte |
| Float (F64) | `-11` | `11` | 8 bytes |
| String | `-12` | N/A | Variable |
| GUID | `-15` | `15` | 16 bytes |

## Literal Syntax

### Integers

```clj
42           ;; I64 (Long) - default
32i          ;; I16 (Short)
1000l        ;; I32 (Int)
0x1a         ;; Byte (hex)
```

### Floating Point

```clj
3.14         ;; F64
1.5e10       ;; Scientific notation
-2.5e-3      ;; Negative exponent
```

### Boolean

```clj
true
false
```

### Symbols

Interned strings for efficient comparison and storage.

```clj
'AAPL        ;; Symbol
'my-symbol   ;; Hyphenated symbol
['A 'B 'C]   ;; Symbol vector
```

### Strings

```clj
"hello world"
"line1\nline2"  ;; With escape sequences
```

### Characters

```clj
'a'          ;; Single character
'Z'
'\n'         ;; Newline char
```

### Date/Time

```clj
;; Date (YYYY.MM.DD)
2025.01.15
2024.12.31

;; Time (HH:MM:SS or HH:MM:SS.mmm)
09:30:00
15:45:30.500

;; Timestamp (Date + Time)
2025.01.15T09:30:00
2025.01.15T09:30:00.123456789
```

### Null Values

```clj
null         ;; Generic null

;; Type-specific nulls
0N           ;; Null (generic)
0Ni          ;; Null I32
0Nl          ;; Null I64
0Nf          ;; Null F64

;; Infinity
0W           ;; Positive infinity
-0W          ;; Negative infinity
```

## Composite Types

### Vectors

Homogeneous collection in square brackets.

```clj
[1 2 3 4 5]              ;; I64 vector
[1.0 2.5 3.14]           ;; F64 vector
['AAPL 'GOOG 'MSFT]      ;; Symbol vector
[true false true]        ;; Boolean vector
[2025.01.01 2025.01.02]  ;; Date vector
```

Empty vector:
```clj
[]
```

### Lists

Heterogeneous collection (mixed types).

```clj
(list 1 'symbol "string" 2025.01.01)
(list [1 2] [3 4])       ;; List of vectors
(list (list 1 2) (list 3 4))  ;; Nested lists
```

### Dictionaries

Key-value mapping. Keys are typically symbols.

```clj
;; Using dict function
(dict [a b c] [1 2 3])

;; Literal syntax
{a: 1 b: 2 c: 3}
{name: "Alice" age: 25 active: true}

;; Mixed value types
{count: 100 price: 99.5 symbol: 'AAPL}
```

### Tables

Relational structure: column names (symbols) → column vectors.

```clj
;; Using table function
(table [id name age] (list
  [1 2 3]
  ['Alice 'Bob 'Charlie]
  [25 30 35]))

;; Columns must be equal length vectors
(table [sym price size] (list
  ['AAPL 'GOOG 'MSFT]
  [150.25 2800.50 310.00]
  [100 50 200]))
```

## Type Conversion

Use `as` function to convert types.

```clj
;; Integer to float
(as 'F64 [1 2 3])        ;; [1.0 2.0 3.0]

;; String to integer
(as 'I64 "123")          ;; 123

;; Symbol to string
(as 'STRING 'hello)      ;; "hello"

;; Date from components
(as 'DATE [2025 1 15])   ;; 2025.01.15
```

### Type Names for Conversion

| Name | Type |
|------|------|
| `'B8` or `'BOOL` | Boolean |
| `'U8` or `'BYTE` | Byte |
| `'I16` or `'SHORT` | Short |
| `'I32` or `'INT` | Int |
| `'I64` or `'LONG` | Long |
| `'F64` or `'FLOAT` | Float |
| `'SYMBOL` | Symbol |
| `'STRING` | String |
| `'DATE` | Date |
| `'TIME` | Time |
| `'TIMESTAMP` | Timestamp |

## Type Checking

```clj
(type 42)           ;; I64
(type [1 2 3])      ;; I64 (vector type)
(type 'sym)         ;; SYMBOL
(type "hello")      ;; STRING
(type {a: 1})       ;; DICT
(type (table ...))  ;; TABLE
```

## Null Handling

Null propagates through operations:

```clj
(+ 5 null)          ;; null
(+ [1 null 3] 10)   ;; [11 null 13]
(> null 10)         ;; null
```

Check for null:

```clj
(nil? null)         ;; true
(nil? 5)            ;; false
(nil? [1 null 3])   ;; [false true false]
```

## Type Coercion Rules

### Arithmetic

- Same types: Result is same type
- Mixed int sizes: Promote to larger
- Int + Float: Promote to Float
- Scalar + Vector: Broadcast scalar

### Comparison

- Returns boolean or boolean vector
- Null comparisons return null

### Aggregation

- `sum`, `avg` on integers → may return float
- `count` always returns I64
- Nulls typically excluded from aggregation

## Vector Broadcasting

Scalar operations broadcast to vectors:

```clj
(+ 10 [1 2 3])           ;; [11 12 13]
(* 2 [1 2 3])            ;; [2 4 6]
(> [1 2 3] 2)            ;; [false false true]
(and [true false] true)  ;; [true false]
```

Vector-vector operations are element-wise:

```clj
(+ [1 2 3] [4 5 6])      ;; [5 7 9]
(* [1 2 3] [2 2 2])      ;; [2 4 6]
```

Vectors must be same length or one must be scalar.

## Memory Layout

Rayfall uses columnar storage:

- Vectors are contiguous memory blocks
- Tables store columns separately
- Zero-copy access for query results
- Reference counting for lifecycle management

This enables efficient:
- SIMD operations on columns
- Cache-friendly sequential access
- Minimal memory copying in queries
