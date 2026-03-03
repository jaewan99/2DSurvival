# Plan: Sight Environment (Backside Darkening)

## Goal
Make the "back side" of the player character appear dark and unclear.
- Player facing right → right/front side is fully visible, left/back side is darkened + desaturated
- Not totally black — dimmed, washed out, slightly detail-less (simulated blur)

## Status
- [x] C++ done — `CharacterMIDs` created in `BeginPlay`, `CharacterForward` pushed in `Tick`
- [ ] Material editor steps (below)

---

## Material Editor Steps

### 1. Add the parameter
- Create a **Vector Parameter** node
- Name: `CharacterForward`
- Default value: `(1, 0, 0, 0)`

---

### 2. Compute BackFactor (0 = front face, 1 = back face)
```
VertexNormalWS  ──┐
                   ├── DotProduct ── Negate ── Clamp(0,1) ──→ [BackFactor]
CharacterForward ──┘
```
- Add a `VertexNormalWS` node (smoother than Pixel Normal WS)
- Add a `Dot Product` node, connect both into it
- Add a `Negate` node after the Dot Product
- Add a `Clamp(min=0, max=1)` after Negate → this is **BackFactor**

---

### 3. Darken the back side
```
BaseColor ──┬────────────────────────────────── Lerp(Alpha=BackFactor) ──→ [DimColor]
            └── Multiply(0.15) ──[dark tint]──┘
```
- `Multiply` the BaseColor by a `Constant` of `0.15` (tune this — lower = darker back)
- `Lerp`: A = original BaseColor, B = multiplied dark color, Alpha = BackFactor

---

### 4. Desaturate for "unclear/foggy" look
```
[DimColor] ──→ Desaturation(Fraction = BackFactor × 0.7) ──→ [FinalColor] ──→ Base Color pin
```
- `Multiply` BackFactor by `0.7` → use as the Fraction input of a `Desaturation` node
- Feed DimColor into the Desaturation input
- Connect result to the **Base Color** pin of the material output

---

### 5. (Optional) Simulated blur — flatten normal map on back side
```
NormalMap ──┐
             ├── Lerp(Alpha = BackFactor × 0.85) ──→ Normal pin
(0,0,1)  ──┘
```
- Add a `Constant3Vector` = `(0, 0, 1)` — this is a flat/featureless normal
- `Multiply` BackFactor by `0.85`
- `Lerp`: A = NormalMap sample, B = (0,0,1), Alpha = result
- Connect to the **Normal** pin of the material output
- Removes surface detail on the back → looks softer, less defined

---

### 6. Assign material
- No extra Blueprint step needed
- C++ creates MIDs from whatever material is already on the mesh slots in `BeginPlay`
- Just make sure `BP_BaseCharacter`'s mesh already has this material assigned in the Details panel

---

## Tuning values
| Parameter | Default | Effect |
|---|---|---|
| Dark multiply | `0.15` | How dark the back is. Raise toward 1.0 for less darkness. |
| Desaturation fraction multiplier | `0.7` | How washed-out the back is. 0 = full color, 1 = greyscale. |
| Normal flatten multiplier | `0.85` | How much surface detail is lost on back. 0 = full detail kept. |
