# Inversion

Requires [Egg v2](https://github.com/aksommerville/egg2) to build.

2026-01-01. For Uplifting Jam #6, theme "CHANGE/TRANSITION".

Single-screen platformer where you can change the environment three ways:
 - Bonk the ceiling to reverse gravity.
 - Duck on a corner to rotate gravity.
 - Press the wall to floop inside of it, and reverse solid/passable.
 
## TODO

- [x] Animate walking and jumping.
- [x] Gravity rotation.
- [x] Wall inversion.
- [x] Goal.
- [x] Gravity feels a bit floaty. Tighten up.
- [x] Hazards.
- [ ] Almost impossible to enter a 1-meter horizontal gap. Should we account for that in the hero's hitbox? Or just avoid in design?
- [x] Render bgbits: OOB are currently "same as me", they need to be "clamp to nearest". Visible in corridors touching the edge.
- [ ] Sound effects.
- [x] Music.
- [ ] Include some kind of Reset option. I won't be able to guarantee that all positions are escapable.
- [ ] Revise graphics. I've been inconsistent about outlines, and not sure what I prefer.
- [ ] Visual fireworks. Dust on landing (+sound effect?). Some kind of "kapow" on gravity changes...
- [ ] Hello modal.
- [ ] Gameover modal.
- [ ] Scorekeeping.
- [ ] Itch page.
