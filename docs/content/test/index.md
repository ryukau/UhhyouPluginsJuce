---
title: "Heading 1 (H1)"
date: 2026-03-16
toc: true
draft: false
---

*Notice the favicon appended after this heading due to the `h1::after` CSS rule.*

## Heading 2 with [a nested link](#)
### Heading 3
#### Heading 4
##### Heading 5
###### Heading 6

## Text Elements and Links A

Here is a standard paragraph containing an [external link](https://example.com) to test `:link`, `:visited`, and hover states. If you need to test inline code, it looks `like this` (which triggers `:not(pre) > code`). To test keyboard inputs, press <kbd>Ctrl</kbd> + <kbd>C</kbd> to copy.

Here is a link with the specific header anchor class: <a href="#test" class="header-anchor">Header Anchor Link</a>.

### Keyboard
<kbd>Esc</kbd> <kbd>F1</kbd> <kbd>F2</kbd> <kbd>F3</kbd> <kbd>F4</kbd> <kbd>F5</kbd> <kbd>F6</kbd> <kbd>F7</kbd> <kbd>F8</kbd> <kbd>F9</kbd> <kbd>F10</kbd> <kbd>F11</kbd> <kbd>F12</kbd> <kbd>PrtSc</kbd> <kbd>Scroll Lock</kbd> <kbd>Pause</kbd>

<kbd>~</kbd> <kbd>!</kbd> <kbd>@</kbd> <kbd>#</kbd> <kbd>$</kbd> <kbd>%</kbd> <kbd>^</kbd> <kbd>&</kbd> <kbd>*</kbd> <kbd>(</kbd> <kbd>)</kbd> <kbd>_</kbd> <kbd>+</kbd> <kbd>{</kbd> <kbd>}</kbd> <kbd>|</kbd> <kbd>:</kbd> <kbd>"</kbd> <kbd>\<</kbd> <kbd>></kbd> <kbd>?</kbd>

<kbd>`</kbd> <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>4</kbd> <kbd>5</kbd> <kbd>6</kbd> <kbd>7</kbd> <kbd>8</kbd> <kbd>9</kbd> <kbd>0</kbd> <kbd>-</kbd> <kbd>=</kbd> <kbd>Backspace</kbd>

<kbd>Tab</kbd> <kbd>Q</kbd> <kbd>W</kbd> <kbd>E</kbd> <kbd>R</kbd> <kbd>T</kbd> <kbd>Y</kbd> <kbd>U</kbd> <kbd>I</kbd> <kbd>O</kbd> <kbd>P</kbd> <kbd>[</kbd> <kbd>]</kbd> <kbd>\\</kbd>

<kbd>Caps Lock</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> <kbd>F</kbd> <kbd>G</kbd> <kbd>H</kbd> <kbd>J</kbd> <kbd>K</kbd> <kbd>L</kbd> <kbd>;</kbd> <kbd>'</kbd> <kbd>Enter</kbd>

<kbd>Shift</kbd> <kbd>Z</kbd> <kbd>X</kbd> <kbd>C</kbd> <kbd>V</kbd> <kbd>B</kbd> <kbd>N</kbd> <kbd>M</kbd> <kbd>,</kbd> <kbd>.</kbd> <kbd>/</kbd> <kbd>Shift</kbd>

<kbd>Ctrl</kbd> <kbd>Win</kbd> <kbd>Alt</kbd> <kbd>Space</kbd> <kbd>AltGr</kbd> <kbd>fn</kbd> <kbd>⌘</kbd> <kbd>⌥</kbd> <kbd>Context Menu</kbd>

<kbd>Insert</kbd> <kbd>Home</kbd> <kbd>PgUp</kbd> <kbd>Delete</kbd> <kbd>End</kbd> <kbd>PgDn</kbd>

<kbd>↑</kbd> <kbd>↓</kbd> <kbd>←</kbd> <kbd>→</kbd>

<kbd>Num Lock</kbd> <kbd>/</kbd> <kbd>*</kbd> <kbd>-</kbd> <kbd>7</kbd> <kbd>8</kbd> <kbd>9</kbd> <kbd>+</kbd> <kbd>4</kbd> <kbd>5</kbd> <kbd>6</kbd> <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>0</kbd> <kbd>.</kbd> <kbd>Enter</kbd>

## Lists

### Standard List
- Item 1 (Tests the `li` margin rule)
- The `::marker` is the first element among the list item's contents.
  - Nested item 1
  - Nested item 2
- Item 3

### Description List

Term 1
: This is the definition for term 1. It should have a dotted border on the left.

Term 2
: This is the definition for term 2.

My Term
: My Definition

### Interactive Details List

<details>
<summary>Click to expand this summary (Tests <code>details > summary</code> and hover)</summary>
<div class="indent">
<p>Here is the hidden content inside the details block.</p>
</div>
</details>

## Tables

| Header Column 1 | Header Column 2 | Header Column 3 |
|:----------------|:----------------|:----------------|
| Row 1, Cell 1   | Row 1, Cell 2   | Row 1, Cell 3   |
| Row 2, Cell 1   | Row 2, Cell 2   | Row 2, Cell 3   |
| Row 3, Cell 1   | Row 3, Cell 2   | Row 3, Cell 3   |
| Row 4, Cell 1   | Row 4, Cell 2   | Row 4, Cell 3   |

*This table tests border spacing, odd/even row background colors, and header/cell borders.*

## Code Blocks

### Standard Preformatted Text (`text`)
```text
This is a standard markdown code block.
It should have an auto overflow and an 8px padding.
```

### Source Code Block
```c++
template<std::floating_point T> class HalfwaveAdaa1 {
private:
  T x1_ = T(0);

public:
  void reset() { x1_ = T(0); }

  T process(T input) {
    const T x_a = x1_;
    const T x_b = input;
    x1_ = input;

    if (x_a <= T(0) && x_b <= T(0)) { return T(0); }
    if (x_a >= T(0) && x_b >= T(0)) { return T(0.5) * (x_a + x_b); }

    const T min_x = std::min(x_a, x_b);
    const T max_x = std::max(x_a, x_b);
    const T abs_diff = max_x - min_x;

    const T integral = T(0.5) * max_x * max_x;
    return integral / abs_diff;
  }
};
```

## Media Elements

### Images
Standard Image (Will invert in dark mode due to `img:not(.staySameInDark)`):
![Placeholder Image](/favicon/favicon.png)

Image with `.staySameInDark` class (Will NOT invert in dark mode):
<img src="/favicon/favicon.png" class="staySameInDark" alt="No invert image">

### Audio and Video
<label for="test-audio">Audio Label Element:</label>
<audio id="test-audio" controls>
  <source src="audio/unaware.opus" type="audio/ogg">
  Your browser does not support the audio tag.
</audio>

<video controls width="100%">
  <source src="video/OvertoneDemo_2024-08-11 22-48-54.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>
