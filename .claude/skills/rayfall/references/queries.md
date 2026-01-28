# Rayfall Query Operations

## SELECT

The primary query operation for filtering, projecting, and aggregating data.

### Basic Syntax

```clj
(select {
  col1: expr1
  col2: expr2
  from: source-table
  where: filter-condition  ;; optional
  by: grouping             ;; optional
})
```

### Simple Select (Projection)

```clj
;; Select specific columns
(select {
  name: name
  salary: salary
  from: employees
})

;; Computed columns
(select {
  name: name
  annual: (* salary 12)
  from: employees
})
```

### Filtering (WHERE)

```clj
;; Single condition
(select {
  name: name
  from: employees
  where: (> salary 70000)
})

;; Multiple conditions
(select {
  name: name
  from: employees
  where: (and (> salary 50000)
              (== dept 'IT))
})

;; Using OR
(select {
  sym: sym
  from: trades
  where: (or (== sym 'AAPL)
             (== sym 'GOOG))
})
```

### Aggregation (BY)

```clj
;; Single grouping column
(select {
  dept: dept
  avg_salary: (avg salary)
  headcount: (count name)
  from: employees
  by: dept
})

;; Multiple grouping columns
(select {
  dept: dept
  region: region
  total: (sum salary)
  from: employees
  by: {dept: dept region: region}
})

;; Aggregation with filter
(select {
  dept: dept
  high_earners: (count name)
  from: employees
  by: dept
  where: (> salary 80000)
})
```

### Available Aggregation Functions

| Function | Description |
|----------|-------------|
| `(count col)` | Count of rows |
| `(sum col)` | Sum of values |
| `(avg col)` | Mean value |
| `(min col)` | Minimum value |
| `(max col)` | Maximum value |
| `(med col)` | Median value |
| `(dev col)` | Standard deviation |
| `(first col)` | First value in group |
| `(last col)` | Last value in group |

## INSERT

Add new rows to a table.

### Single Row

```clj
;; Returns new table
(set employees (insert employees (list 'Charlie 35 'IT 65000)))

;; In-place modification (quoted symbol)
(insert 'employees (list 'Charlie 35 'IT 65000))
```

### Multiple Rows

```clj
;; Column vectors
(set employees (insert employees (list
  ['David 'Eve 'Frank]
  [40 25 32]
  ['HR 'IT 'HR]
  [70000 55000 62000])))
```

### From Dictionary

```clj
(set employees (insert employees (dict
  [name age dept salary]
  (list 'Grace 28 'IT 72000))))
```

## UPDATE

Modify existing rows in a table.

### Basic Update

```clj
;; Update with condition
(set employees (update {
  salary: (* salary 1.1)
  from: employees
  where: (== rating 'A)
}))

;; Update multiple columns
(set employees (update {
  salary: (* salary 1.05)
  bonus: 5000
  from: employees
  where: (> tenure 5)
}))
```

### Update with Grouping

```clj
;; Apply update per group
(set employees (update {
  rank: (rank salary)
  from: employees
  by: dept
}))
```

### In-Place Update

```clj
;; Using quoted symbol
(update {
  status: 'processed
  from: 'orders
  where: (== status 'pending)
})
```

## UPSERT

Insert new rows or update existing ones based on key column.

### Syntax

```clj
(upsert table key-col-index new-data)
```

### Examples

```clj
;; Key is column 0 (id)
;; If id exists, update; else insert
(set users (upsert users 0 (list [1 2 3] ['Alice 'Bob 'Charlie])))

;; Single row upsert
(set users (upsert users 0 (list 4 'David)))

;; In-place
(upsert 'users 0 (list 5 'Eve))
```

## ALTER

Modify individual elements or columns.

### Vector Alteration

```clj
;; Alter at index
(set prices (alter prices + 2 50))  ;; Add 50 at index 2

;; Set specific value
(set prices (alter prices (fn [_] 100) 0))  ;; Set index 0 to 100
```

### Table Column Alteration

```clj
;; Alter entire column
(set trades (alter trades + 'price 10))  ;; Add 10 to all prices

;; In-place
(alter 'trades * 'size 2)  ;; Double all sizes
```

## JOINS

### LEFT JOIN

Keep all rows from left table, match from right.

```clj
(left-join [join-cols] left-table right-table)

;; Example: Join trades with orders on order_id
(left-join [order_id] trades orders)

;; Multiple join columns
(left-join [sym date] positions prices)
```

### INNER JOIN

Only rows that match in both tables.

```clj
(inner-join [join-cols] left-table right-table)

;; Example
(inner-join [customer_id] orders customers)
```

### ASOF JOIN

Time-series join: find greatest value ≤ join key.

```clj
(asof-join [join-cols] left-table right-table)

;; Example: Match trades to most recent quote
(asof-join [sym ts] trades quotes)
```

The join columns must be sorted. For each row in left table, finds the row in right table with the largest key value that is ≤ the left key.

### WINDOW JOIN

Aggregate right table values within a time window around each left row.

```clj
;; First, compute time intervals
(set intervals (map-left + [-1000 1000] (at trades 'ts)))

;; Join with aggregation
(window-join [sym ts] intervals trades quotes {
  min_bid: (min bid)
  max_ask: (max ask)
  avg_spread: (avg (- ask bid))
})
```

The intervals define [start, end] for each trade. All quotes within that window are aggregated.

## Query Patterns

### VWAP Calculation

```clj
(select {
  sym: sym
  vwap: (/ (sum (* price size)) (sum size))
  volume: (sum size)
  from: trades
  by: sym
})
```

### Running Totals

```clj
(set running (select {
  date: date
  cumulative: (scan + amount)
  from: transactions
}))
```

### Top N per Group

```clj
;; Rank within group, then filter
(set ranked (update {
  rank: (rank (neg salary))
  from: employees
  by: dept
}))

(select {
  name: name
  dept: dept
  salary: salary
  from: ranked
  where: (<= rank 3)
})
```

### Pivoting Data

```clj
;; Group by row key, aggregate columns
(select {
  date: date
  aapl_vol: (sum (filter size (== sym 'AAPL)))
  goog_vol: (sum (filter size (== sym 'GOOG)))
  from: trades
  by: date
})
```
