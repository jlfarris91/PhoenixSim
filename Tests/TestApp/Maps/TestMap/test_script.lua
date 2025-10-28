
function OnPreUpdate()
  return 1
end

function OnUpdate()
  return 1
end

function OnPostUpdate()
  return 1
end

function OnPreHandleAction(world, action)

  return false;
end

function OnHandleAction(world, action)

  return false;
end

function OnPostHandleAction(world, action)

  return false;
end

function OnWorldInitialize(world)

  -- Spawn a unit
  entityId = Phoenix.Unit.SpawnUnit(world, 10, hash("Test123"), 64, 64, 45);

  return 1
end

function OnWorldShutdown(world)
  return 1
end

function OnPreWorldUpdate(world)
  return 1
end

function OnWorldUpdate(world)

  -- Apply some force to the unit
  Phoenix.Physics.ApplyForce(world, entityId, 1.5, 1.5)

  return 1
end

function OnPostWorldUpdate(world)
  return 1
end

function OnPreHandleWorldAction(world, action)

  return false;
end

function OnHandleWorldAction(world, action)

  return false;
end

function OnPostHandleWorldAction(world, action)

  return false;
end