# Advanced Building Generation Plan

## Goal

Give each room a typed archetype based on building type and floor, with building-type-specific
wall/floor materials and interactable prop spawning. No two buildings should feel identical.

---

## Building Types

`EBuildingType` gains a 4th value: `Restaurant`.

| Type       | Ground Floor Archetypes        | Upper Floor Archetypes          |
|------------|--------------------------------|---------------------------------|
| House      | Kitchen, LivingRoom            | Bedroom, Bathroom               |
| Hospital   | Reception, WaitingRoom         | PatientWard, OperatingRoom      |
| Store      | ShopFloor, Counter             | StoreRoom, StaffRoom            |
| Restaurant | DiningArea, Bar                | Kitchen, StaffRoom              |

---

## New: ERoomArchetype (enum)

```
Generic, Bedroom, Bathroom, Kitchen, LivingRoom,
Reception, WaitingRoom, PatientWard, OperatingRoom,
ShopFloor, Counter, StoreRoom, StaffRoom,
DiningArea, Bar
```

---

## New: URoomDefinition (UDataAsset)

One asset per archetype per building. Fields:

- `ERoomArchetype Archetype`
- `UMaterialInterface* WallMaterial`   — applied to room actor's wall mesh at runtime
- `UMaterialInterface* FloorMaterial`  — applied to room actor's floor mesh at runtime
- `TArray<FRoomPropEntry> Props`        — what can spawn in this room

### FRoomPropEntry struct

- `TSubclassOf<AActor> PropClass`       — the prop Blueprint (lootable or decorative)
- `float SpawnChance`                   — 0–1, rolled per prop slot
- `ERoomPropPlacement Placement`        — LeftWall, Center, RightWall, Random

### ERoomPropPlacement (enum)

Controls which X position within the room cell the prop spawns at.

---

## New: UBuildingDefinition additions

- `TArray<URoomDefinition*> GroundFloorRooms`  — pick one randomly per ground-floor cell
- `TArray<URoomDefinition*> UpperFloorRooms`   — pick one randomly per upper-floor cell

Remove single `RoomActorClass` field — replaced by the room definition lists above.
`StairsActorClass`, `ElevatorRoomActorClass`, `DoorActorClass` remain unchanged.

---

## New: IRoomActor interface (or BlueprintNativeEvent on a C++ base)

Room actors (BP_RoomCell etc.) must implement:

```cpp
void ApplyRoomDefinition(URoomDefinition* Def)  // BlueprintNativeEvent
```

This is called by the generator after spawn. The BP override sets WallMaterial / FloorMaterial
on the relevant mesh components by index.

---

## Generator Changes (ABuildingGenerator)

1. Pick `URoomDefinition*` for each non-stair, non-elevator room cell:
   - Floor 0 → random from `GroundFloorRooms`
   - Floor 1+ → random from `UpperFloorRooms`
2. Spawn the `RoomActorClass` from the chosen definition (each `URoomDefinition` has its own
   `TSubclassOf<AActor> RoomActorClass` so different archetypes can use different BP meshes).
3. Call `ApplyRoomDefinition` on the spawned actor.
4. Spawn props: iterate `Def->Props`, roll SpawnChance, compute X offset from Placement,
   spawn at `RoomOrigin + PropOffset`. Props are standard actors — lootable ones implement
   `IInteractable` and use the existing `AWorldItem`-style loot pattern.

---

## Lootable Props

A prop that implements `IInteractable`:
- Press E → spawns loot items on the floor (like enemy death drops) and plays an open animation
  or simply destroys itself / hides mesh.
- Configured via a `TArray<FLootEntry> LootTable` (reuse `FLootEntry` from `EnemyBase`).
- One-time use: after looting, prompt changes to "Already searched" or prop is hidden.

Base C++ class: `ALootableProp` (AActor + IInteractable).
Blueprint children: `BP_Cabinet`, `BP_MedicalLocker`, `BP_ShopShelf`, `BP_Fridge`, etc.

---

## Decorative Props ✅ DONE

`ADecorativeProp` (C++) is implemented with two extra properties:
- `SpawnProbability` (float 0–1, default 1.0) — if a random roll fails in `BeginPlay`, actor is hidden + collision disabled.
- `VariantMeshes` (`TArray<TSoftObjectPtr<UStaticMesh>>`, default empty) — if populated, `BeginPlay` picks one at random and sets it on the Mesh component. A null entry keeps the Blueprint default mesh.

Blueprint children: `BP_Chair`, `BP_Table`, `BP_HospitalBed`, `BP_CounterDesk`, etc. (assign mesh only).

**Note on walls**: `ARoomCell` no longer has LeftWall/RightWall/BackWall in C++. Only Floor and Ceiling are in C++ (runtime-positioned by `SetRoomDimensions`). Add walls directly in the Blueprint viewport — they stay at fixed positions relative to the actor root.

---

## Implementation Order

1. Add `Restaurant` to `EBuildingType`
2. ✅ Create `ERoomArchetype` + `ERoomCategory` enums — done in `RoomDefinition.h` (`ERoomPropPlacement` still needed)
3. ✅ Create `FRoomPropEntry` struct and `URoomDefinition` data asset — `URoomDefinition` done; `FRoomPropEntry` + `ERoomPropPlacement` still needed
4. Update `UBuildingDefinition` — add `GroundFloorRooms` / `UpperFloorRooms` arrays (remove single `RoomActorClass`); keep stairs/elevator/door fields
5. Create `ALootableProp` (C++) — IInteractable, LootTable, one-time-loot logic
6. ✅ Create `ADecorativeProp` (C++) — done with `SpawnProbability` + `VariantMeshes`
7. ✅ Add `ApplyRoomDefinition` BlueprintNativeEvent — done on `ARoomCell` (C++ base is no-op; override in Blueprint)
8. Update `ABuildingGenerator::Generate()` — archetype selection + prop spawning
9. Blueprint steps (documented at bottom)

---

## Blueprint Steps (after compile)

- Create `BP_LootableProp` children: `BP_Cabinet`, `BP_MedicalLocker`, `BP_ShopShelf`, `BP_Fridge`
- Create `BP_DecorativeProp` children: `BP_Chair`, `BP_Table`, `BP_HospitalBed`, `BP_CounterDesk`, `BP_BarStool`, `BP_DiningTable`
- Create `DA_Room_*` data assets per archetype (e.g. `DA_Room_Bedroom`, `DA_Room_Kitchen`)
  - Assign wall/floor materials, fill Props array
- Update `DA_Building_House`, `DA_Building_Hospital` etc:
  - Fill `GroundFloorRooms` + `UpperFloorRooms` arrays with the relevant `DA_Room_*` assets
- Room Blueprint (`BP_RoomCell`): override `ApplyRoomDefinition` to call `Set Material` on wall/floor meshes
