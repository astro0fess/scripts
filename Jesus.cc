if not shared.Saved.Conditions then
    shared.Saved.Conditions = {}
end

shared.Saved.Conditions["Whilst a player is selected"] = shared.Saved.Conditions["Whilst a player is selected"] or {
    ["Knocked"] = true,
    ["Self-Knocked"] = true,
    ["Visible"] = true,
}

shared.Saved.Conditions["When selecting a player"] = shared.Saved.Conditions["When selecting a player"] or {
    ["Knocked"] = true,
    ["Self-Knocked"] = true,
    ["Visible"] = true,
}

local whilstSelected = shared.Saved.Conditions["Whilst a player is selected"] or {
    ["Knocked"] = true,
    ["Self-Knocked"] = true,
    ["Visible"] = true,
}

local whenSelecting = shared.Saved.Conditions["When selecting a player"] or {
    ["Knocked"] = true,
    ["Self-Knocked"] = true,
    ["Visible"] = true,
}

shared.Saved.Toggle = shared.Saved['Global'] and shared.Saved['Global']['Keybind'] or "C"
shared.Saved.Mode = "Target"

shared.Saved['Silent Aim']   = shared.Saved['Silent Aimbot'] or {}
shared.Saved['Camera Aimbot'] = shared.Saved['Camera Aimbot'] or {}
shared.Saved['Trigger Bot']  = shared.Saved['Trigger Bot'] or {}

shared.Saved['Silent Aim']['Closest Point'] = shared.Saved['Silent Aim']['Closest Point'] or { Type = 'Advanced', Scale = 0.35 }
shared.Saved['Silent Aim'].FOV = shared.Saved['Silent Aim'].FOV or { Enabled = true, Size = {X=5,Y=5,Z=5} }

shared.Saved['Camera Aimbot']['Camera Aimbot Checks'] = shared.Saved['Camera Aimbot']['Camera Modes'] or {
    ['First Person'] = true,
    ['Third Person'] = false,
    ['Right Click'] = true,
}

local function getToggleKey()
    local toggleChar = shared.Saved.Toggle or "C"
    if type(toggleChar) ~= "string" or toggleChar == "" then
        toggleChar = "C"
    end

    local success, keyCode = pcall(function()
        return Enum.KeyCode[toggleChar]
    end)

    if success and keyCode then
        return keyCode
    else
        warn("Invalid keybind, falling back to C")
        return Enum.KeyCode.C
    end
end

local getCurrentWeaponFOV = function()
    local silent = shared.Saved['Silent Aimbot'] or shared.Saved['Silent Aim'] or {}
    local fovMain = silent.FOV or { Enabled = false, Size = {X=5,Y=5,Z=5} }

    if not fovMain.Enabled then
        return Vector3.new(5, 5, 5)
    end

    local wc = fovMain['Weapon Configuration'] or { Enabled = false }
    if not wc.Enabled then
        local base = fovMain.Size or {X=5,Y=5,Z=5}
        return Vector3.new(base.X or 5, base.Y or 5, base.Z or 5)
    end

    local character = LocalPlayer.Character
    if not character then
        local others = wc.Others or {X=5,Y=5,Z=5}
        return Vector3.new(others.X or 5, others.Y or 5, others.Z or 5)
    end

    local tool = character:FindFirstChildOfClass("Tool")
    if not tool then
        local others = wc.Others or {X=5,Y=5,Z=5}
        return Vector3.new(others.X or 5, others.Y or 5, others.Z or 5)
    end

    local weaponName = tool.Name:gsub("[%[%]]", "")
    local config = wc.Others or {X=5,Y=5,Z=5}

    if ShotgunNames[weaponName] then config = wc.Shotguns or config
    elseif PistolNames[weaponName] then config = wc.Pistols or config end

    return Vector3.new(config.X or 5, config.Y or 5, config.Z or 5)
end

local function safeAnimate(player, animId)
    if not player or not player.Character or not player.Character:FindFirstChild("Humanoid") then
        return
    end

    local humanoid = player.Character.Humanoid
    local animator = humanoid:FindFirstChildOfClass("Animator") or Instance.new("Animator", humanoid)

    pcall(function()
        local track = animator:LoadAnimation(Instance.new("Animation"))
        track.AnimationId = "rbxassetid://" .. tostring(animId or 0)
        track:Play()
    end)
end

loadstring([[function LPH_NO_VIRTUALIZE(f) return f end;]])();

local camlockPaused = false
local cameraTarget = nil
local Players      = game:GetService("Players")
local UserInput    = game:GetService("UserInputService")
local UserInputService = game:GetService("UserInputService")
local Workspace    = game:GetService("Workspace")
local RunService   = game:GetService("RunService")
local Camera       = Workspace.CurrentCamera
local LocalPlayer  = Players.LocalPlayer
local Mouse        = LocalPlayer:GetMouse()
local triggerBox = nil
local lastActivationTime = 0
local triggerKeyDown = false
local triggerToggled = false
local lockedPlayer = nil
local originalIndex
local box = nil
local BOX_VISIBLE = false
local targetKnocked = false
local selfKnocked = false
local camBox = nil

local ShotgunNames = { 
    ["Double-Barrel SG"] = true, 
    ["TacticalShotgun"] = true, 
    ["Shotgun"] = true, 
    ["DrumShotgun"] = true 
}

local PistolNames = { 
    ["Revolver"] = true, 
    ["Silencer"] = true, 
    ["Glock"] = true 
}

local getMousePosition = function()
    return Mouse.X, Mouse.Y
end

local getCurrentWeaponFOV = LPH_NO_VIRTUALIZE(function()
    local silent = shared.Saved['Silent Aim'] or shared.Saved['Silent Aimbot'] or {}
    local fovMain = silent.FOV or { Enabled = false, Size = {X=5,Y=5,Z=5} }
    if not fovMain.Enabled then
        return Vector3.new(0, 0, 0)
    end

    local wc = fovMain['Weapon Configuration'] or { Enabled = false }
    local useWeaponConfig = wc and wc.Enabled

    if not useWeaponConfig then
        local base = fovMain.Size or {X=5,Y=5,Z=5}
        return Vector3.new(base.X or 0, base.Y or 0, base.Z or 0)
    end
    local character = LocalPlayer.Character
    if not character then
        local others = wc.Others or {X=5,Y=5,Z=5}
        return Vector3.new(others.X or 4, others.Y or 6, others.Z or 2)
    end

    local tool = character:FindFirstChildOfClass("Tool")
    if not tool then
        local others = wc.Others or {X=5,Y=5,Z=5}
        return Vector3.new(others.X or 4, others.Y or 6, others.Z or 2)
    end

    local weaponName = tool.Name:gsub("[%[%]]", "")
    local config

    if ShotgunNames[weaponName] then
        config = wc.Shotguns or wc.Others
    elseif PistolNames[weaponName] then
        config = wc.Pistols or wc.Others
    else
        config = wc.Others or {X=5,Y=5,Z=5}
    end

    return Vector3.new(config.X or 4, config.Y or 6, config.Z or 2)
end)

local isVisible = LPH_NO_VIRTUALIZE(function(rootPart)
    local direction = rootPart.Position - Camera.CFrame.Position
    local params = RaycastParams.new()
    params.FilterType = Enum.RaycastFilterType.Blacklist
    params.FilterDescendantsInstances = {LocalPlayer.Character}
    params.IgnoreWater = true
    local result = workspace:Raycast(Camera.CFrame.Position, direction, params)
    return result == nil or result.Instance:IsDescendantOf(rootPart.Parent)
end)

local isTargetVisible = LPH_NO_VIRTUALIZE(function(targetCharacter)
    if not targetCharacter or not targetCharacter:FindFirstChild("Head") then
        return false
    end

    local head = targetCharacter.Head
    local origin = Camera.CFrame.Position
    local direction = (head.Position - origin)
    
    local raycastParams = RaycastParams.new()
    raycastParams.FilterType = Enum.RaycastFilterType.Blacklist
    raycastParams.FilterDescendantsInstances = {LocalPlayer.Character}
    raycastParams.IgnoreWater = true

    local result = workspace:Raycast(origin, direction, raycastParams)
    
    return result == nil or result.Instance:IsDescendantOf(targetCharacter)
end)

local oldRandom
oldRandom = hookfunction(math.random, LPH_NO_VIRTUALIZE(function(...)
    local args = { ... }
    if checkcaller() then
        return oldRandom(...)
    end
    local multiplier = 1
    if shared.Saved['Weapon Modifications'] and shared.Saved['Weapon Modifications'].Enabled then
        local character = LocalPlayer.Character
        if character then
            local tool = character:FindFirstChildOfClass("Tool")
            if tool then
                local weaponName = tool.Name:gsub("[%[%]]", "")
                local weaponMods = shared.Saved['Weapon Modifications']
                
                if weaponMods['[Double-Barrel SG]'] and weaponName == "Double-Barrel SG" then
                    multiplier = weaponMods['[Double-Barrel SG]'].Multiplier or 1
                elseif weaponMods['[TacticalShotgun]'] and weaponName == "TacticalShotgun" then
                    multiplier = weaponMods['[TacticalShotgun]'].Multiplier or 1
                elseif weaponMods['[Shotgun]'] and weaponName == "Shotgun" then
                    multiplier = weaponMods['[Shotgun]'].Multiplier or 1
                elseif weaponMods['[DrumShotgun]'] and weaponName == "DrumShotgun" then
                    multiplier = weaponMods['[DrumShotgun]'].Multiplier or 1
                end
            end
        end
    end

    if
        (#args == 0)
        or (args[1] == -0.05 and args[2] == 0.05)
        or (args[1] == -0.1)
        or (args[1] == -0.05)
    then
        if multiplier ~= 1 then
            return oldRandom(...) * multiplier
        end
    end

    return oldRandom(...)
end))

local updateKnockChecks = LPH_NO_VIRTUALIZE(function()
    local targetChar = lockedPlayer and lockedPlayer.Character
    local selfChar = LocalPlayer.Character
    
    if selfChar and whilstSelected['Self-Knocked'] then
        local selfBodyEffects = selfChar:FindFirstChild("BodyEffects")
        selfKnocked = selfBodyEffects and selfBodyEffects:FindFirstChild("K.O") and selfBodyEffects["K.O"].Value
    else
        selfKnocked = false
    end
    if targetChar and whilstSelected['Knocked'] then
        local targetBodyEffects = targetChar:FindFirstChild("BodyEffects")
        targetKnocked = targetBodyEffects and targetBodyEffects:FindFirstChild("K.O") and targetBodyEffects["K.O"].Value
    else
        targetKnocked = false
    end
end)

local getCamlockFOV = LPH_NO_VIRTUALIZE(function()
    local cam = shared.Saved['Camera Aimbot']
    local fovMain = cam.FOV

    if not fovMain.Enabled then
        return Vector3.new(5,5,5)
    end

    local wc = fovMain['Weapon Configuration']
    if not wc.Enabled then
        local base = fovMain.Size
        return Vector3.new(base.X or 5, base.Y or 5, base.Z or 5)
    end

    local character = LocalPlayer.Character
    if not character then return Vector3.new(5,5,5) end

    local tool = character:FindFirstChildOfClass("Tool")
    if not tool then return Vector3.new(5,5,5) end

    local weaponName = tool.Name:gsub("[%[%]]", "")
    local config = wc.Others
    
    if ShotgunNames[weaponName] then config = wc.Shotguns
    elseif PistolNames[weaponName] then config = wc.Pistols end
    
    return Vector3.new(config.X or 5, config.Y or 5, config.Z or 5)
end)

local getTriggerbotFOV = LPH_NO_VIRTUALIZE(function()
    local tb = shared.Saved['Trigger Bot']
    local fov = tb.FOV
    return Vector3.new(fov.X or 2, fov.Y or 5.2, fov.Z or 1.5)
end)

local isMouseInCamBox = LPH_NO_VIRTUALIZE(function()
    if not (cameraTarget and cameraTarget.Character) then return false end
    local root = cameraTarget.Character:FindFirstChild("HumanoidRootPart")
    if not root then return false end
    
    local boxSize = getCamlockFOV()
    local half = Vector3.new(boxSize.X/2, boxSize.Y/2, boxSize.Z/2)
    local bMin = root.Position - half
    local bMax = root.Position + half
    local ray = Camera:ScreenPointToRay(Mouse.X, Mouse.Y)
    local o, d = ray.Origin, ray.Direction
    local tMin = (bMin - o) / d
    local tMax = (bMax - o) / d
    local t1 = Vector3.new(math.min(tMin.X,tMax.X), math.min(tMin.Y,tMax.Y), math.min(tMin.Z,tMax.Z))
    local t2 = Vector3.new(math.max(tMin.X,tMax.X), math.max(tMin.Y,tMax.Y), math.max(tMin.Z,tMax.Z))
    local tNear = math.max(t1.X, math.max(t1.Y, t1.Z))
    local tFar  = math.min(t2.X, math.min(t2.Y, t2.Z))
    return tNear <= tFar and tFar >= 0
end)

local isMouseInTriggerBox = LPH_NO_VIRTUALIZE(function()
    if not lockedPlayer or not lockedPlayer.Character then return false end
    local root = lockedPlayer.Character:FindFirstChild("HumanoidRootPart")
    if not root then return false end

    local fov = shared.Saved['Trigger Bot'].FOV
    local half = Vector3.new(fov.X or 0, fov.Y or 0, fov.Z or 0) / 2
    local min = root.Position - half
    local max = root.Position + half
    local ray = Camera:ScreenPointToRay(Mouse.X, Mouse.Y)

    local tMin = (min - ray.Origin) / ray.Direction
    local tMax = (max - ray.Origin) / ray.Direction
    local t1 = Vector3.new(math.min(tMin.X, tMax.X), math.min(tMin.Y, tMax.Y), math.min(tMin.Z, tMax.Z))
    local t2 = Vector3.new(math.max(tMin.X, tMax.X), math.max(tMin.Y, tMax.Y), math.max(tMin.Z, tMax.Z))
    local tNear = math.max(t1.X, math.max(t1.Y, t1.Z))
    local tFar  = math.min(t2.X, math.min(t2.Y, t2.Z))

    return tNear <= tFar and tFar >= 0
end)

local getClosestPlayerToMouse = LPH_NO_VIRTUALIZE(function()
    local closest = nil
    local bestDist = math.huge
    
    local mx, my = getMousePosition()
    local mousePos = Vector2.new(mx, my)

    for _, player in Players:GetPlayers() do
        if player ~= LocalPlayer and player.Character then
            local root = player.Character:FindFirstChild("HumanoidRootPart")
            local bodyEffects = player.Character:FindFirstChild("BodyEffects")
            local knocked = bodyEffects and bodyEffects:FindFirstChild("K.O") and bodyEffects["K.O"].Value

            if root and not knocked then
                local screenPos = Camera:WorldToViewportPoint(root.Position)
                if screenPos.Z > 0 and isVisible(root) then
                    local dist = (Vector2.new(screenPos.X, screenPos.Y) - mousePos).Magnitude
                    if dist < bestDist then
                        bestDist = dist
                        closest = player
                    end
                end
            end
        end
    end
    
    return closest
end)

local GetClosestPointAdvanced = LPH_NO_VIRTUALIZE(function(Part, Scale)
    local cf = Part.CFrame
    local size = Part.Size * (Scale / 2)

    local ray = Mouse.UnitRay
    local rel = cf:PointToObjectSpace(ray.Origin + ray.Direction * ray.Direction:Dot(cf.Position - ray.Origin))

    return cf * Vector3.new(
        math.clamp(rel.X, -size.X, size.X),
        math.clamp(rel.Y, -size.Y, size.Y),
        math.clamp(rel.Z, -size.Z, size.Z)
    )
end)

local GetClosestPointBasic = LPH_NO_VIRTUALIZE(function(Part)
    if not Part then return Part.Position end
    local ray = Mouse.UnitRay
    local params = RaycastParams.new()
    params.FilterDescendantsInstances = {Part}
    params.FilterType = Enum.RaycastFilterType.Whitelist
    local result = Workspace:Raycast(ray.Origin, ray.Direction * 1000, params)
    return result and result.Position or Part.Position
end)

local UserInputService = game:GetService("UserInputService")
local Camera = workspace.CurrentCamera

local function GetClosestPartToCursor(Character)
    local bestPart = nil
    local bestDist = math.huge
    local mousePos = UserInputService:GetMouseLocation()

    for _, part in ipairs(Character:GetChildren()) do
        if part:IsA("BasePart") then
            local screenPos, onScreen = Camera:WorldToViewportPoint(part.Position)
            if onScreen then
                local dist = (Vector2.new(screenPos.X, screenPos.Y) - mousePos).Magnitude
                if dist < bestDist then
                    bestDist = dist
                    bestPart = part
                end
            end
        end
    end

    return bestPart or Character:FindFirstChild("HumanoidRootPart") or Character:FindFirstChild("Head")
end

local getClosestPart = LPH_NO_VIRTUALIZE(function(char)
    return GetClosestPartToCursor(char)
end)

local applyPrediction = LPH_NO_VIRTUALIZE(function(partOrNil, basePos)
    if not partOrNil then return basePos end
    
    local silent = shared.Saved['Silent Aimbot'] or shared.Saved['Silent Aim'] or {}
    local pred = silent.Prediction or {X=0,Y=0,Z=0}
    local vel = partOrNil.Velocity or Vector3.new(0,0,0)
    local offset = Vector3.new(
        vel.X * (pred.X or 0),
        vel.Y * (pred.Y or 0), 
        vel.Z * (pred.Z or 0)
    )
    return basePos + offset
end)

local resolveHit = LPH_NO_VIRTUALIZE(function(targetChar)
    local silent = shared.Saved['Silent Aimbot'] or shared.Saved['Silent Aim'] or {}
    local mode = silent['Hit Part'] or "Closest Point"
    local result = nil
    local sourcePart = nil

    if mode == "Closest Point" then
        sourcePart = getClosestPart(targetChar)
        if sourcePart then
            local cp = silent['Closest Point'] or {Type="Advanced", Scale=0.35}
            local scale = cp.Scale or 0.35
            local point = (cp.Type == "Advanced") and
                GetClosestPointAdvanced(sourcePart, scale) or
                GetClosestPointBasic(sourcePart)
            result = point
        end

    elseif mode == "Closest Part" then
        sourcePart = getClosestPart(targetChar)
        if sourcePart then 
            result = sourcePart.Position 
        end

    elseif mode == "Head" then
        local head = targetChar:FindFirstChild("Head")
        if head then
            sourcePart = head
            result = head.Position
        end

    else
        local specific = targetChar:FindFirstChild(mode)
        if specific and specific:IsA("BasePart") then
            sourcePart = specific
            result = specific.Position
        end
    end

    if result and sourcePart then
        result = applyPrediction(sourcePart, result)
    end

    return result, sourcePart
end)

local function canTargetPlayer(player)
    if not player or not player.Character then return false end
    
    local selectCond = whenSelecting
    
    if selectCond["Self Knocked"] then
        local be = LocalPlayer.Character and LocalPlayer.Character:FindFirstChild("BodyEffects")
        if be and be:FindFirstChild("K.O") and be["K.O"].Value then 
            return false 
        end
    end

    if selectCond["Knock Check"] then
        local be = player.Character:FindFirstChild("BodyEffects")
        if be and be:FindFirstChild("K.O") and be["K.O"].Value then 
            return false 
        end
    end
    
    if selectCond["Visible"] and not isTargetVisible(player.Character) then 
        return false 
    end
    
    return true
end

local function createSilentBox(targetPlayer)
    if box then box:Destroy() end
    
    local boxSize = getCurrentWeaponFOV()
    
    box = Instance.new("BoxHandleAdornment")
    box.Size = boxSize
    box.Color3 = Color3.fromRGB(255,0,0)
    box.Transparency = 1
    box.AlwaysOnTop = true
    box.ZIndex = 10
    box.Adornee = targetPlayer.Character
    box.Parent  = targetPlayer.Character
    if not BOX_VISIBLE then box.Transparency = 1 end
end

local function createTriggerBox(targetPlayer)
    if triggerBox then triggerBox:Destroy() end
    local fov = shared.Saved['Trigger Bot'].FOV
    local boxSize = Vector3.new(fov.X or 0, fov.Y or 0, fov.Z or 0)
    
    triggerBox = Instance.new("BoxHandleAdornment")
    triggerBox.Size = boxSize
    triggerBox.Color3 = Color3.fromRGB(0, 150, 255)
    triggerBox.Transparency = 1
    triggerBox.AlwaysOnTop = true
    triggerBox.ZIndex = 12
    triggerBox.Adornee = targetPlayer.Character
    triggerBox.Parent = targetPlayer.Character
end

local function createCamBox(targetPlayer)
    if camBox then camBox:Destroy() end
    local boxSize = getCamlockFOV()
    camBox = Instance.new("BoxHandleAdornment")
    camBox.Size = boxSize
    camBox.Color3 = Color3.fromRGB(0,255,0) 
    camBox.Transparency = 1
    camBox.AlwaysOnTop = true
    camBox.ZIndex = 11
    camBox.Adornee = targetPlayer.Character
    camBox.Parent = targetPlayer.Character
end

local isMouseInBox = LPH_NO_VIRTUALIZE(function()
    if not (lockedPlayer and lockedPlayer.Character) then return false end
    local root = lockedPlayer.Character:FindFirstChild("HumanoidRootPart")
    if not root then return false end

    local boxSize = getCurrentWeaponFOV()
    local half = Vector3.new(boxSize.X/2, boxSize.Y/2, boxSize.Z/2)
    local bMin = root.Position - half
    local bMax = root.Position + half

    local ray = Camera:ScreenPointToRay(Mouse.X, Mouse.Y)
    local o, d = ray.Origin, ray.Direction

    local tMin = (bMin - o) / d
    local tMax = (bMax - o) / d

    local t1 = Vector3.new(math.min(tMin.X,tMax.X), math.min(tMin.Y,tMax.Y), math.min(tMin.Z,tMax.Z))
    local t2 = Vector3.new(math.max(tMin.X,tMax.X), math.max(tMin.Y,tMax.Y), math.max(tMin.Z,tMax.Z))

    local tNear = math.max(t1.X, math.max(t1.Y, t1.Z))
    local tFar  = math.min(t2.X, math.min(t2.Y, t2.Z))

    return tNear <= tFar and tFar >= 0
end)

local function getToggleKey()
    local toggleChar = shared.Saved.Toggle or "C"
    if type(toggleChar) ~= "string" or toggleChar == "" then
        toggleChar = "C"
    end

    local success, keyCode = pcall(function()
        return Enum.KeyCode[toggleChar]
    end)

    if success and keyCode then
        return keyCode
    else
        warn("Invalid keybind, falling back to C")
        return Enum.KeyCode.C
    end
end

local triggerBotActive = false

UserInputService.InputBegan:Connect(LPH_NO_VIRTUALIZE(function(inp, gp)
   if gp then return end

    if inp.KeyCode == getToggleKey() then
        if lockedPlayer then
            lockedPlayer = nil
            cameraTarget = nil
            if box then box:Destroy() box = nil end
            if camBox then camBox:Destroy() camBox = nil end
            if triggerBox then triggerBox:Destroy() triggerBox = nil end
        else
            local closest = getClosestPlayerToMouse()
            if closest and canTargetPlayer(closest) then
                lockedPlayer = closest
                cameraTarget = closest
                createSilentBox(closest)
                createCamBox(closest)
                createTriggerBox(closest)
            end
        end
    end

    local bind = shared.Saved['Trigger Bot']['Activation']['Activation Bind'] or "O"
    if inp.KeyCode == Enum.KeyCode[bind] then
        local mode = shared.Saved['Trigger Bot']['Activation']['Activation Mode'] or "Toggle"

        if mode == "Toggle" then
            triggerBotActive = not triggerBotActive
            triggerToggled = triggerBotActive
        elseif mode == "Hold" then
            triggerKeyDown = true
        end
    end
end))

UserInputService.InputEnded:Connect(LPH_NO_VIRTUALIZE(function(inp, gp)
    if gp then return end
    local bind = shared.Saved['Trigger Bot']['Activation']['Activation Bind'] or "O"
    if inp.KeyCode == Enum.KeyCode[bind] then
        triggerKeyDown = false
    end
end))

originalIndex = hookmetamethod(game, "__index", LPH_NO_VIRTUALIZE(function(self, key)
    if self == Mouse and (key == "Hit" or key == "Target") and lockedPlayer then
        local cond = shared.Saved.Conditions["Whilst a player is selected"] or whilstSelected
        if (not cond["Visible"] or isTargetVisible(lockedPlayer.Character)) and isMouseInBox() then
            local pos, part = resolveHit(lockedPlayer.Character)
            if pos and part then
                if key == "Hit" then
                    return CFrame.new(pos)
                else 
                    return part
                end
            end
        end
    end
    return originalIndex(self, key)
end))

local DeepFakePosition = loadstring(game:HttpGet("https://raw.githubusercontent.com/Nosssa/NossLock/main/GetRealMousePosition"))() 
task.wait()

local China = setmetatable({}, {
    __index = LPH_NO_VIRTUALIZE(function(Company, Price)
        return game:GetService(Price)
    end)
})
   
local ChinaWorld = China.Workspace
local Society = China.Players
local ChineseDeporation = China.ReplicatedStorage
local ChinaInputService = China.UserInputService

local ChingChong = Society.LocalPlayer
local Cat =  "meow!!" and ChingChong:GetMouse()

local ChineseEvent = ChineseDeporation:FindFirstChild("MainEvent") or nil
local Payment = "Hello Da Hoodian!" and nil

local RandomChinese = function(RandomCredit)
   return type(RandomCredit) == "number" and math.random(-RandomCredit, RandomCredit) or 0
end

local ChinaAlive = function(ChinesePlayer)
   return ChinesePlayer and ChinesePlayer.Character and ChinesePlayer.Character:FindFirstChild("Humanoid") and ChinesePlayer.Character:FindFirstChild("Head") or false
end

local GameArgs = {
    [9196894486] = "UpdateMousePos",
}

local DEFAULT_ARG = "UpdateMousePos"
local ChinaHook
ChinaHook = hookmetamethod(game, "__namecall", LPH_NO_VIRTUALIZE(function(self, ...)
    local ChinaArgs       = {...}
    local DeportationMethod = getnamecallmethod()

    local targetArg = GameArgs[game.PlaceId] or DEFAULT_ARG

    if not checkcaller()
    and DeportationMethod == "FireServer"
    and self.Name == "MainEvent"
    and ChinaArgs[1] == targetArg then

        ChinaArgs[2] = _G.FetchPosition()
        return self.FireServer(self, unpack(ChinaArgs))
    end

    return ChinaHook(self, ...)
end))

RunService.RenderStepped:Connect(LPH_NO_VIRTUALIZE(function()
    local selectedCond = whilstSelected

    if shared.Saved['Mode'] == 'Target' and lockedPlayer then
        local selfChar = LocalPlayer.Character
        local targetChar = lockedPlayer.Character
        local shouldUnlock = false

        if selectedCond["Self Knocked"] then
            local be = selfChar and selfChar:FindFirstChild("BodyEffects")
            if be and be:FindFirstChild("K.O") and be["K.O"].Value then
                shouldUnlock = true
            end
        end

        if selectedCond["Knock Check"] then
            local be = targetChar and targetChar:FindFirstChild("BodyEffects")
            if be and be:FindFirstChild("K.O") and be["K.O"].Value then
                shouldUnlock = true
            end
        end

        if selectedCond["Visible"] and not isTargetVisible(targetChar) then
        end

        if shouldUnlock then
            lockedPlayer = nil
            cameraTarget = nil
            if box then box:Destroy() box = nil end
            if camBox then camBox:Destroy() camBox = nil end
            if triggerBox then triggerBox:Destroy() triggerBox = nil end
            return
        end
    end

    if shared.Saved['Mode'] == 'Auto' then
        local closestToMouse = getClosestPlayerToMouse()
        
        if closestToMouse and closestToMouse.Character and canTargetPlayer(closestToMouse) then
            lockedPlayer = closestToMouse
            cameraTarget = closestToMouse
            
            if not box then createSilentBox(closestToMouse) end
            if not camBox then createCamBox(closestToMouse) end
            if not triggerBox then createTriggerBox(closestToMouse) end
        else
            lockedPlayer = nil
            cameraTarget = nil
            if box then box:Destroy() box = nil end
            if camBox then camBox:Destroy() camBox = nil end
            if triggerBox then triggerBox:Destroy() triggerBox = nil end
        end
    end

    if lockedPlayer and lockedPlayer.Character then
        cameraTarget = lockedPlayer
        local pos, part = resolveHit(lockedPlayer.Character)
        if pos then
        end
    end

    if box and lockedPlayer and lockedPlayer.Character then
        box.Size = getCurrentWeaponFOV()
        box.Adornee = lockedPlayer.Character
    end

    if camBox and cameraTarget and cameraTarget.Character then
        camBox.Size = getCamlockFOV()
        camBox.Adornee = cameraTarget.Character
    end

    if triggerBox and lockedPlayer and lockedPlayer.Character then
        local fov = shared.Saved['Trigger Bot'].FOV
        triggerBox.Size = Vector3.new(fov.X or 0, fov.Y or 0, fov.Z or 0)
        triggerBox.Adornee = lockedPlayer.Character
    end

    if lockedPlayer and shared.Saved['Trigger Bot'].Enabled and (not selectedCond["Visible"] or isTargetVisible(lockedPlayer.Character)) then
        local tb = shared.Saved['Trigger Bot']
        local mode = tb['Activation']['Activation Mode'] or "Toggle"
        local cooldown = tb['Click Cooldown'] or 0

        local char = LocalPlayer.Character
        local hasKnife = char and (char:FindFirstChild("[Knife]") or char:FindFirstChild("Knife"))

        if not hasKnife then
            local inBox = isMouseInTriggerBox()
            local active = false

            if mode == "Always" then 
                active = true 
            elseif mode == "Hold" then 
                active = triggerKeyDown 
            elseif mode == "Toggle" then 
                active = triggerBotActive
            end

            if inBox and active then
                local now = tick()
                if now - lastActivationTime >= cooldown then
                    local tool = char and char:FindFirstChildOfClass("Tool")
                    if tool then
                        tool:Activate()
                        lastActivationTime = now
                    end
                end
            end
        end
    end

    if cameraTarget and cameraTarget.Character and shared.Saved['Camera Aimbot'].Enabled and (not selectedCond["Visible"] or isTargetVisible(cameraTarget.Character)) and isMouseInCamBox() then
        local camConfig = shared.Saved['Camera Aimbot']
        local checks = camConfig['Camera Aimbot Checks'] or camConfig['Camera Modes'] or {}
        local zoomDist = (Camera.CFrame.Position - Camera.Focus.Position).Magnitude
        local isFirstPerson = zoomDist < 1
        local isThirdPerson = zoomDist >= 1
        local rightClickHeld = UserInputService:IsMouseButtonPressed(Enum.UserInputType.MouseButton2)

        local fpAllowed = checks['First Person'] and isFirstPerson
        local tpAllowed = checks['Third Person'] and isThirdPerson
        local rcAllowed = not checks['Right Click'] or rightClickHeld
        local targetVisible = true

        if selectedCond["Visible"] then
            local head = cameraTarget.Character:FindFirstChild("Head")
            if head then
                local rayParams = RaycastParams.new()
                rayParams.FilterType = Enum.RaycastFilterType.Blacklist
                rayParams.FilterDescendantsInstances = {LocalPlayer.Character, cameraTarget.Character}
                rayParams.IgnoreWater = true
                local result = Workspace:Raycast(Camera.CFrame.Position, (head.Position - Camera.CFrame.Position), rayParams)
                targetVisible = result == nil or result.Instance:IsDescendantOf(cameraTarget.Character)
            end
        end

        if (fpAllowed or tpAllowed) and rcAllowed and targetVisible then
            local hitPartName = camConfig['Hit Part']
            local targetPart
            if hitPartName == "Closest Part" then
                targetPart = getClosestPart(cameraTarget.Character)
            else
                targetPart = cameraTarget.Character:FindFirstChild(hitPartName)
                if not targetPart then
                    targetPart = cameraTarget.Character:FindFirstChild("Head")
                end
            end
            if targetPart then
                local pred = camConfig.Prediction
                local vel = targetPart.Velocity
                local predictedPos = targetPart.Position + Vector3.new(
                    vel.X * pred.X,
                    vel.Y * pred.Y,
                    vel.Z * pred.Z
                )
               
                local snappiness = camConfig.Snappiness or 0.1
                local direction = (predictedPos - Camera.CFrame.Position).Unit
                local currentLook = Camera.CFrame.LookVector
                local newLook = currentLook:lerp(direction, snappiness)
                Camera.CFrame = CFrame.new(Camera.CFrame.Position, Camera.CFrame.Position + newLook)
            end
        end
    end
end))
