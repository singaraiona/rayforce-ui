# Rayfall Language Skill

Comprehensive reference for the Rayfall query language used with RayforceDB and RayLens.

## What is Rayfall?

Rayfall is a pure LISP-like language designed for columnar data analytics. It powers:
- **RayforceDB**: High-performance columnar database engine
- **RayLens**: Real-time analytics dashboard application

## Key Features

- Simple LISP syntax with minimal parentheses
- Vectorized operations (automatic broadcasting)
- First-class tables, vectors, and dictionaries
- Powerful query operations (select, join, aggregate)
- Zero-copy data binding for UI components

## Files

- `SKILL.md` - Main skill reference with syntax, types, and examples
- `references/types.md` - Detailed type system documentation
- `references/functions.md` - Complete function reference
- `references/queries.md` - Query operation details (SELECT, INSERT, UPDATE, JOIN)

## Quick Example

```clj
;; Create a table
(set trades (table [sym ts price size] (list
  ['AAPL 'GOOG 'AAPL 'MSFT]
  [09:30:00 09:30:01 09:30:02 09:30:03]
  [150.25 2800.50 150.30 310.00]
  [100 50 200 150])))

;; Query with aggregation
(select {
  sym: sym
  vwap: (/ (sum (* price size)) (sum size))
  volume: (sum size)
  from: trades
  by: sym
})
```

## When to Use This Skill

- Writing `.rfl` files for RayLens dashboards
- Creating queries for RayforceDB
- Understanding Rayfall syntax and semantics
- Debugging Rayfall expressions
- Learning Rayfall idioms and patterns
