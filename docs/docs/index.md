---
hide:
  - navigation
  - toc
---

<div class="hero-section" markdown>
<div class="hero-content" markdown>

<div class="scroll-fade-in">
<img src="images/logo_dark_full.svg" alt="Rayforce UI" class="hero-logo light-only">
<img src="images/logo_light_full.svg" alt="Rayforce UI" class="hero-logo dark-only">
</div>

# Native **GUI** for RayforceDB {.scroll-fade-in}

<div class="hero-description scroll-fade-in">
  Grids, charts, and REPL powered by Rayfall expressions. Zero-copy rendering directly from columnar buffers — no serialization, no copies, no overhead.
</div>

<div class="hero-buttons scroll-fade-in">
<a href="https://github.com/nicholasgasior/rayforce-ui/releases" class="md-button md-button--primary">Download
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
<line x1="5" y1="12" x2="19" y2="12"></line>
<polyline points="12 5 19 12 12 19"></polyline>
</svg>
</a>
<a href="https://github.com/nicholasgasior/rayforce-ui" class="md-button" target="_blank">GitHub</a>
</div>

<div class="hero-socials scroll-fade-in" markdown>
[:fontawesome-brands-github:](https://github.com/nicholasgasior/rayforce-ui "GitHub")
[:fontawesome-brands-x-twitter:](https://x.com/RayforceDB "X (Twitter)")
</div>

</div>

<div class="hero-ribbon">
<div class="ribbon-particles"></div>
<div class="ribbon-wave"></div>
<div class="ribbon-track">
<div class="ribbon-content">
<span class="ribbon-item"><span class="ribbon-icon">&#9638;</span> Virtualized Grids</span>
<span class="ribbon-item"><span class="ribbon-icon">&#9708;</span> Real-Time Charts</span>
<span class="ribbon-item"><span class="ribbon-icon">&gt;_</span> Interactive REPL</span>
<span class="ribbon-item"><span class="ribbon-icon">&#9889;</span> Zero-Copy Rendering</span>
<span class="ribbon-item"><span class="ribbon-icon">&#128274;</span> MIT License</span>
<span class="ribbon-item"><span class="ribbon-icon">&#9654;</span> ImGui + ImPlot</span>
<span class="ribbon-item"><span class="ribbon-icon">&#128295;</span> Pure C Core</span>
<span class="ribbon-item"><span class="ribbon-icon">&#123;&#125;</span> Rayfall Powered</span>
</div>
<div class="ribbon-content" aria-hidden="true">
<span class="ribbon-item"><span class="ribbon-icon">&#9638;</span> Virtualized Grids</span>
<span class="ribbon-item"><span class="ribbon-icon">&#9708;</span> Real-Time Charts</span>
<span class="ribbon-item"><span class="ribbon-icon">&gt;_</span> Interactive REPL</span>
<span class="ribbon-item"><span class="ribbon-icon">&#9889;</span> Zero-Copy Rendering</span>
<span class="ribbon-item"><span class="ribbon-icon">&#128274;</span> MIT License</span>
<span class="ribbon-item"><span class="ribbon-icon">&#9654;</span> ImGui + ImPlot</span>
<span class="ribbon-item"><span class="ribbon-icon">&#128295;</span> Pure C Core</span>
<span class="ribbon-item"><span class="ribbon-icon">&#123;&#125;</span> Rayfall Powered</span>
</div>
</div>
</div>
</div>

<div class="features-section" markdown>

## Built for **speed** {.scroll-fade-in}

<div class="grid cards scroll-fade-in" markdown>

- :material-table:{ .lg .middle .feature-icon } **Grid**

    ---

    Virtualized table display with row selection and column sorting — renders millions of rows directly from columnar buffers.

- :material-chart-line:{ .lg .middle .feature-icon } **Chart**

    ---

    Real-time line, bar, scatter, and OHLC candle charts via ImPlot with zero-copy data binding from Rayforce queries.

- :material-console:{ .lg .middle .feature-icon } **REPL**

    ---

    Interactive Rayfall console with command history, file loading, and inline result rendering for live data exploration.

- :material-lightning-bolt:{ .lg .middle .feature-icon } **Zero-Copy**

    ---

    Pointer exchange between Rayforce and UI threads. No serialization, no marshaling — raw `obj_p` column buffers rendered directly.

</div>

</div>

<div class="code-demo-section">

<div class="code-demo-header scroll-fade-in">
<h2>Widgets are <strong>Rayfall expressions</strong></h2>
<p>Create widgets, push data, and attach callbacks — all in Rayfall.</p>
</div>

<div class="code-tabs scroll-fade-in">
<div class="code-tab-list">
<button class="code-tab active" data-tab="widget">Widget</button>
<button class="code-tab" data-tab="draw">Draw</button>
<button class="code-tab" data-tab="callback">Callback</button>
<button class="code-tab" data-tab="live">Live Dashboard</button>
</div>

<div class="code-panels">
<div class="code-panel active" data-panel="widget">
<div class="code-block">
<pre><code><span class="code-comment">;; Create a grid widget — opens a docked panel</span>
<span class="code-paren">(</span><span class="code-keyword">set</span> grid1 <span class="code-paren">(</span><span class="code-func">widget</span> <span class="code-paren">{</span>type: <span class="code-symbol">'grid</span> name: <span class="code-string">"trades"</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span>

<span class="code-comment">;; Create a chart widget</span>
<span class="code-paren">(</span><span class="code-keyword">set</span> chart1 <span class="code-paren">(</span><span class="code-func">widget</span> <span class="code-paren">{</span>type: <span class="code-symbol">'chart</span> name: <span class="code-string">"Price Chart"</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span>

<span class="code-comment">;; Create a text widget for summary values</span>
<span class="code-paren">(</span><span class="code-keyword">set</span> count1 <span class="code-paren">(</span><span class="code-func">widget</span> <span class="code-paren">{</span>type: <span class="code-symbol">'text</span> name: <span class="code-string">"Trade Count"</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span></code></pre>
</div>
</div>

<div class="code-panel" data-panel="draw">
<div class="code-block">
<pre><code><span class="code-comment">;; Push query results to a grid</span>
<span class="code-paren">(</span><span class="code-func">draw</span> grid1 <span class="code-paren">(</span><span class="code-func">select</span> <span class="code-paren">{</span>from: trades where: <span class="code-paren">(</span><span class="code-func">&gt;</span> price <span class="code-number">100</span><span class="code-paren">)</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span>

<span class="code-comment">;; Push aggregated data to a chart</span>
<span class="code-paren">(</span><span class="code-func">draw</span> chart1 <span class="code-paren">(</span><span class="code-func">select</span> <span class="code-paren">{</span>
  from: <span class="code-paren">(</span><span class="code-func">take</span> trades <span class="code-number">-500</span><span class="code-paren">)</span>
  by: <span class="code-paren">(</span><span class="code-func">xbar</span> Ts <span class="code-number">1000</span><span class="code-paren">)</span>
  open: <span class="code-paren">(</span><span class="code-func">first</span> Price<span class="code-paren">)</span>
  high: <span class="code-paren">(</span><span class="code-func">max</span> Price<span class="code-paren">)</span>
  low: <span class="code-paren">(</span><span class="code-func">min</span> Price<span class="code-paren">)</span>
  close: <span class="code-paren">(</span><span class="code-func">last</span> Price<span class="code-paren">)</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span>

<span class="code-comment">;; Push scalar to a text widget</span>
<span class="code-paren">(</span><span class="code-func">draw</span> count1 <span class="code-paren">(</span><span class="code-func">count</span> trades<span class="code-paren">)</span><span class="code-paren">)</span></code></pre>
</div>
</div>

<div class="code-panel" data-panel="callback">
<div class="code-block">
<pre><code><span class="code-comment">;; Widget with row-select callback</span>
<span class="code-paren">(</span><span class="code-keyword">set</span> grid1 <span class="code-paren">(</span><span class="code-func">widget</span> <span class="code-paren">{</span>
  type: <span class="code-symbol">'grid</span>
  name: <span class="code-string">"symbols"</span>
  on-select: <span class="code-paren">(</span><span class="code-keyword">fn</span> <span class="code-paren">[</span>row<span class="code-paren">]</span>
    <span class="code-paren">(</span><span class="code-func">draw</span> details row<span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span>

<span class="code-comment">;; User clicks a row → callback fires</span>
<span class="code-comment">;; UI sends expression string to Rayforce thread</span>
<span class="code-comment">;; Rayforce evaluates, allocates obj_p</span>
<span class="code-comment">;; Next draw renders the updated data</span></code></pre>
</div>
</div>

<div class="code-panel" data-panel="live">
<div class="code-block">
<pre><code><span class="code-comment">;; Create tables and simulate live market data</span>
<span class="code-paren">(</span><span class="code-keyword">set</span> trades <span class="code-paren">(</span><span class="code-func">table</span> <span class="code-paren">[</span>Sym Ts Price<span class="code-paren">]</span>
  <span class="code-paren">(</span><span class="code-func">list</span> <span class="code-paren">(</span><span class="code-func">as</span> <span class="code-symbol">'SYMBOL</span> <span class="code-paren">[</span><span class="code-paren">]</span><span class="code-paren">)</span> <span class="code-paren">(</span><span class="code-func">as</span> <span class="code-symbol">'TIME</span> <span class="code-paren">[</span><span class="code-paren">]</span><span class="code-paren">)</span> <span class="code-paren">[</span><span class="code-paren">]</span><span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">)</span>

<span class="code-paren">(</span><span class="code-func">timer</span> <span class="code-number">20</span> <span class="code-number">1000000000</span> <span class="code-paren">(</span><span class="code-keyword">fn</span> <span class="code-paren">[</span>x<span class="code-paren">]</span>
  <span class="code-paren">(</span><span class="code-func">insert</span> <span class="code-symbol">'trades</span> <span class="code-paren">(</span><span class="code-func">list</span>
    <span class="code-paren">(</span><span class="code-func">at</span> <span class="code-paren">[</span>AAPL IBM MSFT GOOG<span class="code-paren">]</span> <span class="code-paren">(</span><span class="code-func">first</span> <span class="code-paren">(</span><span class="code-func">rand</span> <span class="code-number">1</span> <span class="code-number">3</span><span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">)</span>
    <span class="code-paren">(</span><span class="code-func">time</span> <span class="code-symbol">'utc</span><span class="code-paren">)</span>
    <span class="code-paren">(</span><span class="code-func">first</span> <span class="code-paren">(</span><span class="code-func">rand</span> <span class="code-number">1</span> <span class="code-number">1000</span><span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">)</span>

<span class="code-comment">;; Widgets + timer-driven updates</span>
<span class="code-paren">(</span><span class="code-keyword">set</span> t <span class="code-paren">(</span><span class="code-func">widget</span> <span class="code-paren">{</span>type: <span class="code-symbol">'grid</span> name: <span class="code-string">"Trades"</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span>
<span class="code-paren">(</span><span class="code-keyword">set</span> pc <span class="code-paren">(</span><span class="code-func">widget</span> <span class="code-paren">{</span>type: <span class="code-symbol">'chart</span> name: <span class="code-string">"Price Chart"</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span>

<span class="code-paren">(</span><span class="code-func">timer</span> <span class="code-number">100</span> <span class="code-number">10000000</span> <span class="code-paren">(</span><span class="code-keyword">fn</span> <span class="code-paren">[</span>x<span class="code-paren">]</span>
  <span class="code-paren">(</span><span class="code-func">draw</span> t <span class="code-paren">(</span><span class="code-func">take</span> trades <span class="code-number">-100</span><span class="code-paren">)</span><span class="code-paren">)</span>
  <span class="code-paren">(</span><span class="code-func">draw</span> pc <span class="code-paren">(</span><span class="code-func">select</span> <span class="code-paren">{</span>
    from: <span class="code-paren">(</span><span class="code-func">take</span> trades <span class="code-number">-500</span><span class="code-paren">)</span>
    by: <span class="code-paren">(</span><span class="code-func">xbar</span> Ts <span class="code-number">1000</span><span class="code-paren">)</span>
    open: <span class="code-paren">(</span><span class="code-func">first</span> Price<span class="code-paren">)</span> high: <span class="code-paren">(</span><span class="code-func">max</span> Price<span class="code-paren">)</span>
    low: <span class="code-paren">(</span><span class="code-func">min</span> Price<span class="code-paren">)</span> close: <span class="code-paren">(</span><span class="code-func">last</span> Price<span class="code-paren">)</span><span class="code-paren">}</span><span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">)</span><span class="code-paren">)</span></code></pre>
</div>
</div>
</div>
</div>

<script>
document.addEventListener('DOMContentLoaded', function() {
  document.querySelectorAll('.code-tab').forEach(tab => {
    tab.addEventListener('click', function() {
      document.querySelectorAll('.code-tab').forEach(t => t.classList.remove('active'));
      document.querySelectorAll('.code-panel').forEach(p => p.classList.remove('active'));
      this.classList.add('active');
      const panel = document.querySelector(`.code-panel[data-panel="${this.dataset.tab}"]`);
      if (panel) panel.classList.add('active');
    });
  });
});
</script>

</div>

<div class="features-section" markdown>

## How it **works** {.scroll-fade-in}

<div class="grid cards scroll-fade-in" markdown>

- :material-swap-horizontal:{ .lg .middle .feature-icon } **Two-Thread Architecture**

    ---

    UI thread runs GLFW + ImGui. Rayforce thread evaluates queries. They communicate via lock-free queues exchanging `obj_p` pointers.

- :material-memory:{ .lg .middle .feature-icon } **Refcount Lifecycle**

    ---

    Data stays alive via refcount while the UI renders. No races, no locks on the hot path — just pointer exchange and `drop_obj` when done.

- :material-code-braces:{ .lg .middle .feature-icon } **Post-Query Pipeline**

    ---

    Any Rayfall expression can be applied to widget data before rendering — filter, aggregate, sort — all evaluated in the Rayforce thread.

</div>

</div>


<div class="cta-section scroll-fade-in" markdown>

## Get started in **seconds**

Download a pre-built binary, run `rayforce-ui examples/basic.rfl`, and watch live grids and charts appear.

[Download](https://github.com/nicholasgasior/rayforce-ui/releases){ .md-button .md-button--primary }
[View Source](https://github.com/nicholasgasior/rayforce-ui){ .md-button }

</div>

<style>
.scroll-fade-in {
  opacity: 0;
  transform: translateY(30px);
  transition: opacity 0.8s ease-out, transform 0.8s ease-out;
}
.scroll-fade-in.visible {
  opacity: 1;
  transform: translateY(0);
}
.hero-content > .scroll-fade-in:first-child,
.hero-ticker.scroll-fade-in {
  opacity: 1 !important;
  transform: translateY(0) !important;
  transition-delay: 0s !important;
}
.hero-content > .scroll-fade-in:first-child img {
  opacity: 1 !important;
}
.hero-content > h1.scroll-fade-in,
.hero-content > .hero-description.scroll-fade-in {
  transition-delay: 0.2s;
}
.hero-content > .hero-buttons.scroll-fade-in {
  transition-delay: 0.3s;
}
.hero-content > .hero-socials.scroll-fade-in {
  transition-delay: 0.4s;
}
</style>

<script>
document.addEventListener('DOMContentLoaded', function() {
  const observer = new IntersectionObserver(function(entries) {
    entries.forEach(entry => {
      if (entry.isIntersecting) entry.target.classList.add('visible');
    });
  }, { threshold: 0.1, rootMargin: '0px 0px -50px 0px' });

  document.querySelectorAll('.scroll-fade-in').forEach(el => {
    if (!el.matches('.hero-content > .scroll-fade-in:first-child') &&
        !el.matches('.hero-ticker.scroll-fade-in')) {
      observer.observe(el);
    } else {
      el.classList.add('visible');
    }
  });
});
</script>
