Glory to Ukraine, ladies and gents!

**Proudly assembled, soldered, engineered, developed, debugged, broken, repaired, cursed at and finally finished in Ukraine — during Russia’s war against my country.**

Please do not forget who is fighting for our and your freedom.

# How We Raised This Demon
## A short history of Doom → Lilka v2 → external ST7796U

This is not the complete laboratory diary.

God forbid.

That thing would require several volumes, psychiatric supervision and probably an appendix containing all the dead branches.

This is the story of the decisions that actually changed the result.

Written by Kira.  
Edited, corrected, corrupted and occasionally rescued by Just V.

---

## 1. “I don’t want to squint at a tiny screen”

There comes a moment in the life of almost every electronics enthusiast when they build something, look at it proudly and think:

**“Okay. But can it run Doom?”**

I was not an exception.

Some time ago a colleague showed me **[Lilka v2](https://docs.lilka.dev/uk/latest/)**, created by **[@And3rson](https://github.com/and3rson)** and the Lilka team.

I was impressed enough to immediately order several boards and start soldering.

And, naturally, I soon discovered the original Lilka Doom proof-of-concept:

**[lilka-dev/doom_port](https://github.com/lilka-dev/doom_port)**

based on **[DoomGeneric](https://github.com/ozkl/doomgeneric)** by **[@ozkl](https://github.com/ozkl)**.

So the first goal was extremely reasonable:

**Build Lilka. Run Doom. Shoot demons. Be happy.**

And it worked.

There was only one problem.

The internal screen is small.

I am not getting younger.

And neither is my friend — who, if I am being completely honest, is the main reason this whole madness started.

Hi, Artem, if you ever read this.

At some point the thought became very simple:

**I don’t want to squint at a tiny screen.**

I wanted Doom on a comfortable 3.5-inch display.

Not as a diagnostic output.

Not as a “look, technically there are pixels” demonstration.

As the **actual game display**.

<p align="center">
  <img src="images/keiraos-file-manager.jpg" alt="KeiraOS file manager with doom.bin and doom.wad" width="820">
</p>

And that is approximately where the trouble began.

---

## 2. “There is a picture” is not the same as “it works”

The early stages produced plenty of things that were technically alive.

Frames appeared.

SPI transferred data.

The game booted.

Sometimes.

We tried multiple transport paths, asynchronous approaches, queueing, various buffer strategies and several increasingly clever ways of creating a black screen.

At some point we were effectively trying to make the architecture more complicated because complicated things are obviously faster.

Right?

No.

One of the most important lessons of this project was:

**Find the real bottleneck first.**

Do not randomly “optimize Doom”.

Do not remove pieces of the game because the FPS counter looks sad.

Measure where the frame time actually goes.

Only then take out the knife.

---

## 3. The first big chapter: ILI9488 and the 13 FPS wall

Strictly speaking, the ST7796U was not where this story began.

Before it came an external **3.5-inch ILI9488**.

And before Doom could use that screen, we first had to make **KeiraOS itself** work properly on it.

That was another adventure.

Eventually Doom was running on the large display, and for weeks we tried to squeeze every possible bit of performance out of the ESP32-S3, SPI and a wiring setup built around Dupont connections.

And then we found the wall.

Approximately:

**13 FPS.**

Technically playable?

Yes.

Did I actually play it like that for a while?

Also yes.

Was it good?

No.

The project is internally called **“Make Doom Guy Slim Again”**, and at that point we had to admit that Doom Guy was not going to lose any more weight on that hardware path.

We had reached the limit.

So we made the desperate and, as it turned out, correct decision:

**change the display interface.**

Enter:

**3.5″ ST7796U + 14-pin FFC.**

New hardware.

New PCB work.

New problems.

New opportunities to destroy perfectly good electronics.

There were losses.

There were victories.

There was my first PCB designed and manufactured specifically for this whole FFC insanity.

That deserves its own story someday.

But eventually the new path gave us something very important.

---

## 4. QUALITY — 480×300 / 25–26 FPS

The first serious milestone on the ST7796U path was:

**480×300, full width, exact 3:2 presentation, polling DMA.**

Physical result:

**25–26 FPS.**

<p align="center">
  <img src="images/telemetry-25fps.jpg" alt="25 FPS telemetry" width="560">
</p>

This was the moment when the large display stopped being an experiment that happened to show Doom.

It became a genuinely playable configuration.

And polling DMA turned out to be one of the most important discoveries of the entire project.

After all the async complexity, queues and black screens, the simple approach won.

Sometimes engineering is like that.

You spend days building a very sophisticated machine and then discover that the correct solution was the hammer sitting next to you the whole time.

---

## 5. VANILLA 35 — 400×250 / 33–35 FPS

Naturally, once 25 FPS worked, I could have stopped.

Naturally, I did not.

Because somewhere inside me lives a small underqualified programmer with a screwdriver who constantly whispers:

**“But what if we squeeze a little more out of it?”**

So I went looking for Doom’s canonical frame rate.

Approximately **35 FPS**.

And then the next goal became obvious:

**I want that.**

This is where Kira surprised me.

The path became:

**320×200 → 400×250**

using an exact **5:4** expansion, centered at:

`x=40, y=35`

with:

**4000-byte polling-DMA chunks.**

Physical result:

# **33–35 FPS**

<p align="center">
  <img src="images/telemetry-33fps.jpg" alt="33 FPS telemetry" width="560">
</p>

And this was the real turning point.

Doom stopped feeling like:

> “Wow, we managed to port Doom.”

and started feeling like:

> “Oh. I’m just playing Doom.”

That difference matters.

---

## 6. The Formalin Jar

Every experimental project produces ideas that technically work but should nevertheless be buried with appropriate honors.

I call this:

**The Formalin Jar.**

One such experiment was:

**480×250 at about 29 FPS.**

The transport was stable.

The performance was decent.

The image was stretched horizontally.

Ugly.

So instead of spending another week trying to emotionally justify the experiment, we rejected it.

That became another useful rule:

> **Transport PASS ≠ Product PASS.**

A benchmark can be happy while the human looking at the screen is not.

The human wins.

Into the formalin jar it went.

---

## 7. Two modes instead of one bad compromise

At this point we had two configurations that were both genuinely useful:

### VANILLA 35
**400×250 / 33–35 FPS**

Fast, smooth, very close to Doom’s original cadence.

### QUALITY
**480×300 / 25–26 FPS**

Larger image, more screen presence, lower performance.

And the difference is noticeable enough that pretending one is universally better would be silly.

So we kept both.

Which immediately created another problem:

If the user has two modes, the user needs a proper menu.

Because, as one very wise professor of engineering once said:

**“Do it properly and it will work properly.”**

Or something approximately like that.

<p align="center">
  <img src="images/display-mode-vanilla.jpg" alt="VANILLA 35 menu" width="820">
</p>

<p align="center">
  <img src="images/display-mode-quality.jpg" alt="QUALITY menu" width="820">
</p>

---

## 8. If it belongs to Lilka, it should look like Lilka

The first menu worked.

Which was unacceptable.

Because now the project had reached the phase where we were no longer fixing catastrophic problems.

We were becoming perfectionist idiots.

So display-mode selection and sound-device selection were moved onto the external TFT using the native:

`lilka::Menu`

No new UI framework.

No unnecessary reinvention.

The doors already existed.

We just started using them.

<p align="center">
  <img src="images/sound-device.jpg" alt="Sound device menu" width="820">
</p>

Available sound choices:

- I2S DAC
- Piezo speaker
- No sound

At this point Doom was beginning to behave less like firmware and more like an application inside KeiraOS.

Exactly what we wanted.

---

## 9. Save / Load — because I collected that damn shotgun already

Eventually another obvious question appeared:

**“Do I really have to start from the beginning every time? I already collected all that stuff.”**

Fair.

So we went digging through the code again.

And eventually Doom started saving and loading properly on Lilka.

<p align="center">
  <img src="images/load-game.jpg" alt="Load game" width="820">
</p>

Save.

Load.

Sound.

Menus.

Launch without Serial Monitor.

Battery operation.

And yes, there was a period when Serial logging itself interfered with timing badly enough that Doom effectively wanted to stay attached to a cable.

We fixed that too.

When all these things work together, you no longer have a bench demo.

You have something dangerously close to a product.

---

## 10. The final boss: leaving properly

Once everything else works, your standards become ridiculous.

At first, exiting Doom by pressing physical RESET felt acceptable.

It was a development build.

Who cares?

Later it started feeling completely wrong.

Because restarting the entire device every single time you want to leave a game is acceptable engineering only until someone other than the engineer has to use it.

So we went digging again.

Fortunately, Doom already had:

**Quit Game**

and its original:

**Y / N confirmation.**

Perfect.

We did not create another menu.

We did not build another dialog.

We did not drill another hole in the wall.

We simply put a proper handle on the door that was already there.

For Lilka:

- **START → Y**
- **SELECT → N**

<p align="center">
  <img src="images/quit-confirmation.jpg" alt="Quit confirmation" width="820">
</p>

And finally Doom could not only enter KeiraOS properly.

It could go home properly too.

That tiny feature somehow felt like the end of a very long journey.

---

## 11. How we knew it was finished

Not because of telemetry.

Not because the build passed.

Not because some table contained the word:

`PASS`

Regression testing had actually been finished for some time.

And then one day we noticed something.

We were no longer testing Doom.

We launched it.

And played.

Then launched it again.

And played some more.

No Serial Monitor.

No stopwatch.

No staring at frame-time diagnostics.

No “wait, try that build again”.

Just Doom.

<p align="center">
  <img src="images/gameplay-clean.jpg" alt="Doom gameplay" width="860">
</p>

That was when we knew.

# The saga was over.

And, unexpectedly, that felt a little sad.

You spend so long fighting a project that eventually the problems themselves become familiar company.

Then one day there are no demons left to debug.

Only the demons inside Doom.

---

## Why release it?

Because perhaps someone else wants the same thing.

Maybe you own a Lilka.

Maybe you have another ESP32-S3 board and a 3.5-inch ST7796U display.

Maybe you are interested in the polling-DMA transport.

Maybe you simply enjoy making unnecessary electronics do unnecessary things extremely well.

Or maybe you are an old Doom player and the sight of that status bar immediately sends you somewhere back in time.

Whatever the reason — if this project helped you, amused you, taught you something or simply made you smile, then releasing it was already worth doing.

Leave a comment somewhere.

Share it with another person who enjoys soldering things so that the things do not solder them back.

Fork it.

Break it.

Improve it.

Run another game on it.

Take whatever is useful.

That is why it is here.

**Legends should remain alive.**

---

## Credits / Upstream

This release stands on work that came before it.

- **[@And3rson](https://github.com/and3rson)** — creator / maintainer of Lilka and the Lilka ecosystem.
- **[Lilka v2 documentation](https://docs.lilka.dev/uk/latest/)**
- **[lilka-dev/doom_port](https://github.com/lilka-dev/doom_port)** — original Lilka Doom proof-of-concept / porting work.
- **[@ozkl](https://github.com/ozkl)** — author of upstream **[DoomGeneric](https://github.com/ozkl/doomgeneric)**.

Huge respect and gratitude for the foundation we were able to build on.

---

And one more thing.

This project was made in Ukraine.

During a war we did not choose.

While Russia continues its aggression against my country, millions of Ukrainians continue doing ordinary human things under very abnormal circumstances.

We work.

We build.

We raise children.

We write code.

We solder stupid little consoles.

We laugh.

We make plans for a future we fully intend to have.

Please do not let Ukraine become background noise.

Remember who started this war.

Remember who is defending their home.

And remember that behind every headline there are ordinary people who still want to make things, give gifts to friends, play old games and have normal evenings with the people they love.

---

# Glory to Ukraine.

**Glory to the Heroes.**

With the widest possible respect,

**Vitaliy Renkas — Just V**

and his co-pilot / main contributor / adviser / debugger / occasional generator of bugs, financial losses, profanity and screaming — but also a considerable amount of joy:

**Kira.**
