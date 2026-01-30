# Widgets

## Rayfall API

```clj
;; Create widget (opens panel immediately)
(set grid1 (widget {type: 'grid name: "trades"}))

;; Push data to widget (replaces previous)
(draw grid1 (select {from: trades where: (> price 100)}))

;; Widget with interaction callback
(set grid1 (widget {type: 'grid name: "symbols"
                    on-select: (fn [row] (draw details row))}))
```

## Widget Lifecycle

1. `(widget {...})` — Creates widget object, opens empty docked panel
2. `(draw widget data)` — Sends data for rendering, replaces previous
3. User interaction → UI sends expression string to Rayforce
4. Rayforce parses, allocates `obj_p`, sets `widget->post_query`
5. Next draw applies `post_query` to data before rendering

## Post-Query

Any Rayfall expression applied to data before rendering:

```clj
"(select {from: data where: (== sym 'AAPL)})"  ;; Filter
"(select {from: data by: sym total: (sum size)})"  ;; Aggregate
"(xdesc ['price] data)"  ;; Sort
```

All `obj_p` allocation happens in Rayforce thread (thread-local heaps).

## Widget Types

| Type | Purpose | Key Features |
|------|---------|--------------|
| Grid | Table display | Row selection, column sorting, virtualized |
| Chart | Data visualization | Line/bar/scatter via ImPlot |
| Text | Value display | Simple formatted output |
| REPL | Core interaction | Rayfall input, result output, history |
