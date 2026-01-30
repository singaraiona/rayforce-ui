# Demo

This demo runs a live market data feed and renders it in RayforceUI — grids, text widgets, and a candlestick chart, all updating in real time.

## Prerequisites

- `rayforce` binary in your `PATH`
- `rayforce-ui` binary in your `PATH`

## Step 1 — Start the Feed Server

Launch the Rayforce server that generates simulated trades and quotes:

```bash
./rayforce examples/srvfeed.rfl -i
```

This starts a Rayforce instance that:

- Creates `trades` and `quotes` tables
- Inserts random market data every second (6 symbols: AAPL, IBM, MSFT, BABA, GOOG, AMZN)
- Listens on port **5110** for client connections

Keep this terminal open.

## Step 2 — Launch RayforceUI

In a second terminal, start the GUI:

```bash
./rayforce-ui
```

Once the window opens, load the client script:

1. Open the REPL panel
2. Use **File → Open** (or the file chooser) to load `examples/clifeed.rfl`

The script connects to the feed server and creates five widgets:

| Widget | Type | Content |
|--------|------|---------|
| Trades | Grid | Last 100 trades |
| Quotes | Grid | Last 100 quotes |
| Trades Count | Text | Total trade count |
| Quotes Count | Text | Total quote count |
| Price Chart | Chart | OHLC candlestick bars from last 500 trades |

All widgets update immediately as the server pushes new data.

## How It Works

The feed server (`srvfeed.rfl`) generates data and broadcasts to connected clients. The client script (`clifeed.rfl`) subscribes with a pair of functions:

- **Server-side function** — runs on the server, selects and aggregates data
- **Client-side function** — runs locally, draws results to widgets

Data flows as zero-copy `obj_p` pointers — no serialization between the Rayforce thread and the UI render loop.
