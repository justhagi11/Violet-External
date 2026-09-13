#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace scripts {

    struct SkyboxData {
        std::string SkyboxUp;
        std::string SkyboxRt;
        std::string SkyboxLf;
        std::string SkyboxFt;
        std::string SkyboxBk;
        std::string SkyboxDn;
    };

    inline std::unordered_map<std::string, SkyboxData> skyboxes = {
        {"Piss", {"rbxassetid://2651437350", "rbxassetid://2651436979", "rbxassetid://2651436494", "rbxassetid://2651435990", "rbxassetid://2651432901", "rbxassetid://2651434974"}},
        {"Peach", {"rbxassetid://566616187", "rbxassetid://566616082", "rbxassetid://566616044", "rbxassetid://566616141", "rbxassetid://566616113", "rbxassetid://566616232"}},
        {"Saku", {"http://www.roblox.com/asset/?id=271077958", "http://www.roblox.com/asset/?id=271042467", "http://www.roblox.com/asset/?id=271042310", "http://www.roblox.com/asset/?id=271042556", "http://www.roblox.com/asset/?id=271042310", "http://www.roblox.com/asset/?id=271077243"}},
        {"Purple", {"http://www.roblox.com/asset/?id=570557727", "http://www.roblox.com/asset/?id=570557672", "http://www.roblox.com/asset/?id=570557620", "http://www.roblox.com/asset/?id=570557559", "http://www.roblox.com/asset/?id=570557514", "http://www.roblox.com/asset/?id=570557775"}},
        {"Retro", {"rbxassetid://18164890128", "rbxassetid://18164873920", "rbxassetid://18164877945", "rbxassetid://18164870251", "rbxassetid://18164881924", "rbxassetid://18166113875"}},
        {"Space", {"rbxassetid://15983964246", "rbxassetid://15983966246", "rbxassetid://15983967420", "rbxassetid://15983965025", "rbxassetid://15983968922", "rbxassetid://15983966825"}},
        {"Sea", {"http://www.roblox.com/asset/?id=321846070", "http://www.roblox.com/asset/?id=321846207", "http://www.roblox.com/asset/?id=321846162", "http://www.roblox.com/asset/?id=321845951", "http://www.roblox.com/asset/?id=321846018", "http://www.roblox.com/asset/?id=321846104"}},
        {"Night V2", {"http://www.roblox.com/Asset/?ID=12064131", "http://www.roblox.com/Asset/?ID=12064115", "http://www.roblox.com/Asset/?ID=12063984", "http://www.roblox.com/Asset/?ID=12064121", "http://www.roblox.com/Asset/?ID=12064107", "http://www.roblox.com/Asset/?ID=12064152"}},
        {"Dark", {"rbxassetid://15470160563", "rbxassetid://15470158022", "rbxassetid://15470155938", "rbxassetid://15470153860", "rbxassetid://15470149279", "rbxassetid://15470151245"}},
        {"Anime", {"http://www.roblox.com/asset/?id=104038404823203", "http://www.roblox.com/asset/?id=99961685452126", "http://www.roblox.com/asset/?id=84924000207295", "http://www.roblox.com/asset/?id=95687237979398", "http://www.roblox.com/asset/?id=81858382098344", "http://www.roblox.com/asset/?id=138472117789684"}},
        {"Beach", {"http://www.roblox.com/asset/?id=151165227", "http://www.roblox.com/asset/?id=151165206", "http://www.roblox.com/asset/?id=151165191", "http://www.roblox.com/asset/?id=151165224", "http://www.roblox.com/asset/?id=151165214", "http://www.roblox.com/asset/?id=151165197"}},
        {"Space V2", {"http://www.roblox.com/asset/?id=16262366016", "http://www.roblox.com/asset/?id=16262363873", "http://www.roblox.com/asset/?id=16262362003", "http://www.roblox.com/asset/?id=16262360469", "http://www.roblox.com/asset/?id=16262356578", "http://www.roblox.com/asset/?id=16262358026"}},
        {"Pink", {"rbxassetid://12635316856", "rbxassetid://12635315817", "rbxassetid://12635313718", "rbxassetid://12635312870", "rbxassetid://12635309703", "rbxassetid://12635311686"}},
        {"Rainbow", {"rbxassetid://12877083856", "rbxassetid://12877085497", "rbxassetid://12877085497", "rbxassetid://12877085497", "rbxassetid://12877085497", "rbxassetid://12877086914"}},
        {"Forest", {"http://www.roblox.com/asset/?id=237593929", "http://www.roblox.com/asset/?id=237593835", "http://www.roblox.com/asset/?id=237593861", "http://www.roblox.com/asset/?id=237593922", "http://www.roblox.com/asset/?id=237593887", "http://www.roblox.com/asset/?id=237593849"}},
        {"Night", {"http://www.roblox.com/asset/?id=154185031", "http://www.roblox.com/asset/?id=154184972", "http://www.roblox.com/asset/?id=154184943", "http://www.roblox.com/asset/?id=154185021", "http://www.roblox.com/asset/?id=154185004", "http://www.roblox.com/asset/?id=154184960"}},
        {"Lava", {"http://www.roblox.com/asset/?id=4776130793", "http://www.roblox.com/asset/?id=4776133150", "http://www.roblox.com/asset/?id=4776128425", "http://www.roblox.com/asset/?id=4776131365", "http://www.roblox.com/asset/?id=4776124334", "http://www.roblox.com/asset/?id=4776125375"}},
        {"Rainy", {"http://www.roblox.com/asset/?id=4495867486", "http://www.roblox.com/asset/?id=4495866584", "http://www.roblox.com/asset/?id=4495866035", "http://www.roblox.com/asset/?id=4495865458", "http://www.roblox.com/asset/?id=4495864450", "http://www.roblox.com/asset/?id=4495864887"}},
        {"Green", {"rbxassetid://566611218", "rbxassetid://566611300", "rbxassetid://566611266", "rbxassetid://566611142", "rbxassetid://566611187", "rbxassetid://566613198"}},
        {"Volcanic", {"http://www.roblox.com/asset/?id=150281471", "http://www.roblox.com/asset/?id=150281426", "http://www.roblox.com/asset/?id=150281400", "http://www.roblox.com/asset/?id=150281461", "http://www.roblox.com/asset/?id=150281446", "http://www.roblox.com/asset/?id=150281418"}},
        {"Minecraft", {"http://www.roblox.com/asset/?id=8735166729", "http://www.roblox.com/asset/?id=8735166751", "http://www.roblox.com/asset/?id=8735166755", "http://www.roblox.com/asset/?id=8735231668", "rbxassetid://8735166756", "http://www.roblox.com/asset/?id=8735166707"}},
        {"Lucid", {"rbxassetid://8508112781", "rbxassetid://8508111092", "rbxassetid://8508107681", "rbxassetid://8508104949", "rbxassetid://8508098796", "rbxassetid://8508103588"}},
        {"Nebulous", {"rbxassetid://131036626982613", "rbxassetid://103716549795832", "rbxassetid://126542804346203", "rbxassetid://107665368823185", "rbxassetid://95020137072033", "rbxassetid://92862258103959"}}
    };

    static const std::string skybox_script_template = R"LUA(
local skyTextures = {
    ["SkyboxBk"] = "%BK%",
    ["SkyboxDn"] = "%DN%",
    ["SkyboxFt"] = "%FT%",
    ["SkyboxLf"] = "%LF%",
    ["SkyboxRt"] = "%RT%",
    ["SkyboxUp"] = "%UP%"
}
local Lighting = game:GetService("Lighting")
local currentSky = Lighting:FindFirstChildOfClass("Sky")
if not currentSky then
    currentSky = Instance.new("Sky")
    currentSky.Parent = Lighting
end
for property, assetId in pairs(skyTextures) do
    pcall(function() currentSky[property] = assetId end)
end
currentSky.SunAngularSize = 10
currentSky.CelestialBodiesShown = true
)LUA";

    static const std::string avatar_script = R"LUA(
getgenv().___miku_target_avatar = "%TARGET%"
local Players = game:GetService("Players")
local player = Players.LocalPlayer

getgenv().___miku_avatar_apply = function(char)
    if not char then return end
    local hum = char:WaitForChild("Humanoid", 15)
    if not hum then return end
    local target = getgenv().___miku_target_avatar
    if not target or target == "" then return end
    local success, targetId = pcall(function() return tonumber(target) or Players:GetUserIdFromNameAsync(target) end)
    if not success or not targetId then return end

    local appearance = Players:GetCharacterAppearanceAsync(targetId)
    if not appearance then return end

    for _, v in pairs(char:GetChildren()) do
        if v:IsA("Accessory") or v:IsA("Shirt") or v:IsA("Pants") or v:IsA("ShirtGraphic") or v:IsA("CharacterMesh") or v:IsA("BodyColors") then
            v:Destroy()
        end
    end

    for _, item in pairs(appearance:GetChildren()) do
        if item:IsA("Shirt") or item:IsA("Pants") or item:IsA("BodyColors") or item:IsA("CharacterMesh") or item:IsA("ShirtGraphic") then
            item.Parent = char
        elseif item:IsA("Accessory") then
            local handle = item:FindFirstChild("Handle")
            if handle then
                item.Parent = char
                local attachment = handle:FindFirstChildOfClass("Attachment")
                if attachment then
                    local targetAttachment = char:FindFirstChild(attachment.Name, true)
                    if targetAttachment then
                        local weld = Instance.new("Weld")
                        weld.Name = "AccessoryWeld"
                        weld.Part0 = handle
                        weld.Part1 = targetAttachment.Parent
                        weld.C0 = attachment.CFrame
                        weld.C1 = targetAttachment.CFrame
                        weld.Parent = handle
                    end
                else
                    local weld = Instance.new("Weld")
                    weld.Part0 = handle
                    weld.Part1 = char:FindFirstChild("Head")
                    if weld.Part1 then
                        weld.C0 = CFrame.new(0, -0.6, 0)
                        weld.Parent = handle
                    end
                end
            end
        end
    end
    appearance:Destroy()
end

if not getgenv().___miku_avatar_hooked then
    getgenv().___miku_avatar_hooked = true
    player.CharacterAdded:Connect(function(char)
        task.wait(1.5)
        pcall(getgenv().___miku_avatar_apply, char)
    end)
end

task.spawn(function()
    for i = 1, 3 do
        local char = player.Character
        if char then pcall(getgenv().___miku_avatar_apply, char) end
        task.wait(3)
    end
end)
)LUA";

    static const std::string username_script = R"LUA(
getgenv().___miku_target_user = "%USER%"
getgenv().___miku_target_display = "%DISPLAY%"
if not getgenv().___miku_user_loop then
    getgenv().___miku_user_loop = true
    task.spawn(function()
        local Players = game:GetService("Players")
        local player = Players.LocalPlayer
        while not player.Character or not player:FindFirstChild("PlayerGui") do task.wait(0.1) end

        local realU = player.Name
        local realD = player.DisplayName
        local realID = tostring(player.UserId)

        while task.wait(0.5) do
            local targetU = getgenv().___miku_target_user
            local targetD = getgenv().___miku_target_display
            if targetD == "" then targetD = targetU end

            local success, targetID = pcall(function() return tostring(Players:GetUserIdFromNameAsync(targetU)) end)
            if not success then targetID = "1" end

            local function escape(str) return str:gsub("[%^%$%(%)%%%.%[%]%*%+%-%?]", "%%%1") end
            local escU, escD = escape(realU), escape(realD)

            local roots = {player:FindFirstChild("PlayerGui"), player.Character, game:GetService("CoreGui")}
            for _, root in pairs(roots) do
                pcall(function()
                    if not root then return end
                    for _, obj in pairs(root:GetDescendants()) do
                        pcall(function()
                            if obj:IsA("TextLabel") or obj:IsA("TextButton") or obj:IsA("TextBox") then
                                local t = obj.Text
                                if t:find(realU) or t:find(realD) then
                                    local nt = t
                                    if realU == realD then

                                        nt = nt:gsub("@" .. escU, "@" .. targetU)
                                        nt = nt:gsub(escU, targetD)
                                    else

                                        if #realU > #realD then
                                            nt = nt:gsub(escU, targetU):gsub(escD, targetD)
                                        else
                                            nt = nt:gsub(escD, targetD):gsub(escU, targetU)
                                        end
                                    end
                                    if obj.Text ~= nt then obj.Text = nt end
                                end
                            elseif obj:IsA("ImageLabel") or obj:IsA("ImageButton") then
                                local img = obj.Image
                                if img:find(realID) or img:find("avatar") or img:find("headshot") or img:find("thumbnail") then
                                    local ni = img:gsub(realID, targetID)
                                    if img:find("id=") then
                                        ni = img:gsub("id=%d+", "id="..targetID):gsub(realID, targetID)
                                    end
                                    if obj.Image ~= ni then obj.Image = ni end
                                end
                            end
                        end)
                    end
                end)
            end
        end
    end)
end
)LUA";

    static const std::string ws_script = R"LUA(
getgenv().___miku_target_ws = %WS%
if not getgenv().___miku_ws_loop then
    getgenv().___miku_ws_loop = true
    task.spawn(function()
        local player = game:GetService("Players").LocalPlayer
        while task.wait(0.5) do
            player:SetAttribute("StatisticDuelsWinStreak", getgenv().___miku_target_ws)
        end
    end)
end
)LUA";

    static const std::string level_script = R"LUA(
getgenv().___miku_target_lvl = %LEVEL%
if not getgenv().___miku_lvl_loop then
    getgenv().___miku_lvl_loop = true
    task.spawn(function()
        local player = game:GetService("Players").LocalPlayer
        while task.wait(0.5) do
            player:SetAttribute("Level", getgenv().___miku_target_lvl)
        end
    end)
end
)LUA";

    static const std::string elo_script = R"LUA(
getgenv().___miku_target_elo = %ELO%
if not getgenv().___miku_elo_loop then
    getgenv().___miku_elo_loop = true
    task.spawn(function()
        local player = game:GetService("Players").LocalPlayer
        while task.wait(0.5) do
            player:SetAttribute("DisplayElo", getgenv().___miku_target_elo)
        end
    end)
end
)LUA";

    static const std::string skin_part_1 = R"LUA(
local lp = game:GetService("Players").LocalPlayer
print("[HEXWARE] Starting Barebones Inject...")

-- [RIVALS BACKEND HOOKING ONLY - NO AC BYPASS]
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local playerScripts = lp:WaitForChild("PlayerScripts")
local controllers = playerScripts:WaitForChild("Controllers")

local EnumLibrary = require(ReplicatedStorage.Modules:WaitForChild("EnumLibrary", 10))
if EnumLibrary then EnumLibrary:WaitForEnumBuilder() end
local CosmeticLibrary = require(ReplicatedStorage.Modules:WaitForChild("CosmeticLibrary", 10))
local ItemLibrary = require(ReplicatedStorage.Modules:WaitForChild("ItemLibrary", 10))
local DataController = require(controllers:WaitForChild("PlayerDataController", 10))

local equipped = {}
local constructingWeapon = nil

local function cloneCosmetic(name, cosmeticType)
    local base = CosmeticLibrary.Cosmetics[name]
    if not base then return nil end
    local data = {}
    for key, value in pairs(base) do data[key] = value end
    data.Name = name
    data.Type = data.Type or cosmeticType
    data.Seed = data.Seed or math.random(1, 1000000)
    if EnumLibrary then
        local success, enumId = pcall(EnumLibrary.ToEnum, EnumLibrary, name)
        if success and enumId then data.Enum, data.ObjectID = enumId, data.ObjectID or enumId end
    end
    return data
end

-- Force ownership locally
CosmeticLibrary.OwnsCosmeticNormally = function() return true end
CosmeticLibrary.OwnsCosmeticUniversally = function() return true end
CosmeticLibrary.OwnsCosmeticForWeapon = function() return true end

local originalOwnsCosmetic = CosmeticLibrary.OwnsCosmetic
CosmeticLibrary.OwnsCosmetic = function(self, inventory, name, weapon)
    if name:find("MISSING_") then return originalOwnsCosmetic(self, inventory, name, weapon) end
    return true
end

local originalGet = DataController.Get
DataController.Get = function(self, key)
    if key == "CosmeticInventory" then
        local proxy = {}
        local data = originalGet(self, key)
        if data then for k, v in pairs(data) do proxy[k] = v end end
        return setmetatable(proxy, {__index = function() return true end})
    end
    return originalGet(self, key)
end

local originalGetWeaponData = DataController.GetWeaponData
DataController.GetWeaponData = function(self, weaponName)
    local data = originalGetWeaponData(self, weaponName)
    if not data then return nil end
    local merged = {}
    for key, value in pairs(data) do merged[key] = value end
    merged.Name = weaponName
    if equipped[weaponName] then
        for cosmeticType, cosmeticData in pairs(equipped[weaponName]) do merged[cosmeticType] = cosmeticData end
    end
    return merged
end
)LUA";

    static const std::string skin_part_2 = R"LUA(
local ClientItem
pcall(function() ClientItem = require(lp.PlayerScripts.Modules.ClientReplicatedClasses.ClientFighter.ClientItem) end)

if ClientItem and ClientItem._CreateViewModel then
    local originalCreateViewModel = ClientItem._CreateViewModel
    ClientItem._CreateViewModel = function(self, viewmodelRef)
        local weaponName = self.Name
        local weaponPlayer = self.ClientFighter and self.ClientFighter.Player
        constructingWeapon = (weaponPlayer == lp) and weaponName or nil    
        
        if weaponPlayer == lp and equipped[weaponName] and equipped[weaponName].Skin and viewmodelRef then
            local dataKey, skinKey, nameKey = self:ToEnum("Data"), self:ToEnum("Skin"), self:ToEnum("Name")
            if viewmodelRef[dataKey] then
                viewmodelRef[dataKey][skinKey] = equipped[weaponName].Skin
                viewmodelRef[dataKey][nameKey] = equipped[weaponName].Skin.Name
            elseif viewmodelRef.Data then
                viewmodelRef.Data.Skin = equipped[weaponName].Skin
                viewmodelRef.Data.Name = equipped[weaponName].Skin.Name
            end
        end
        local result = originalCreateViewModel(self, viewmodelRef)
        constructingWeapon = nil
        return result
    end
end

local viewModelModule = lp.PlayerScripts.Modules.ClientReplicatedClasses.ClientFighter.ClientItem:FindFirstChild("ClientViewModel")
if viewModelModule then
    local ClientViewModel = require(viewModelModule)
    local originalNew = ClientViewModel.new
    ClientViewModel.new = function(replicatedData, clientItem)
        local weaponPlayer = clientItem.ClientFighter and clientItem.ClientFighter.Player
        local weaponName = constructingWeapon or clientItem.Name
        
        if weaponPlayer == lp and equipped[weaponName] then
            local ReplicatedClass = require(ReplicatedStorage.Modules.ReplicatedClass)
            local dataKey = ReplicatedClass:ToEnum("Data")
            replicatedData[dataKey] = replicatedData[dataKey] or {}
            local cosmetics = equipped[weaponName]
            if cosmetics.Skin then replicatedData[dataKey][ReplicatedClass:ToEnum("Skin")] = cosmetics.Skin end
        end
        return originalNew(replicatedData, clientItem)
    end
end

local originalGetViewModelImage = ItemLibrary.GetViewModelImageFromWeaponData
ItemLibrary.GetViewModelImageFromWeaponData = function(self, weaponData, highRes)
    if not weaponData then return originalGetViewModelImage(self, weaponData, highRes) end
    local weaponName = weaponData.Name
    if equipped[weaponName] and equipped[weaponName].Skin then
        local skinInfo = self.ViewModels[equipped[weaponName].Skin.Name]
        if skinInfo then return skinInfo[highRes and "ImageHighResolution" or "Image"] or skinInfo.Image end
    end
    return originalGetViewModelImage(self, weaponData, highRes)
end

-- [EXECUTE THE SWAP]
local TargetWeapon = "Assault Rifle"
local TargetSkin = "Boneclaw Rifle"

equipped[TargetWeapon] = equipped[TargetWeapon] or {}
local clonedSkinData = cloneCosmetic(TargetSkin, "Skin")

if clonedSkinData then
    equipped[TargetWeapon]["Skin"] = clonedSkinData
    print("[HEXWARE] Successfully injected " .. TargetSkin)
else
    print("[HEXWARE] ERROR: Could not find skin in CosmeticLibrary.")
end

task.defer(function()
    pcall(function() DataController.CurrentData:Replicate("WeaponInventory") end)
end)
)LUA";

    static const std::string skin_script = skin_part_1 + skin_part_2;

    static const std::string rivals_unlock_script = skin_script;


    inline std::string sunc_script = R"LUA(loadstring(game:HttpGet("https://raw.githubusercontent.com/sens6/sunc/main/sunc.lua"))())LUA";

    inline std::string unc_script = R"LUA(
local passes, fails, undefined = 0, 0, 0
local running = 0

local function getGlobal(path)
	local value = getfenv(0)

	while value ~= nil and path ~= "" do
		local name, nextValue = string.match(path, "^([^.]+)%.?(.*)$")
		value = value[name]
		path = nextValue
	end

	return value
end

local function test(name, aliases, callback)
	running += 1

	task.spawn(function()
		if not callback then
			print("⏺️ " .. name)
		elseif not getGlobal(name) then
			fails += 1
			warn("⛔ " .. name)
		else
			local success, message = pcall(callback)

			if success then
				passes += 1
				print("✅ " .. name .. (message and " • " .. message or ""))
			else
				fails += 1
				warn("⛔ " .. name .. " failed: " .. message)
			end
		end

		local undefinedAliases = {}

		for _, alias in ipairs(aliases) do
			if getGlobal(alias) == nil then
				table.insert(undefinedAliases, alias)
			end
		end

		if #undefinedAliases > 0 then
			undefined += 1
			warn("⚠️ " .. table.concat(undefinedAliases, ", "))
		end

		running -= 1
	end)
end

print("\n")

print("UNC Environment Check")
print("✅ - Pass, ⛔ - Fail, ⏺️ - No test, ⚠️ - Missing aliases\n")

task.defer(function()
	repeat task.wait() until running == 0

	local rate = math.round(passes / (passes + fails) * 100)
	local outOf = passes .. " out of " .. (passes + fails)

	print("\n")

	print("UNC Summary")
	print("✅ Tested with a " .. rate .. "% success rate (" .. outOf .. ")")
	print("⛔ " .. fails .. " tests failed")
	print("⚠️ " .. undefined .. " globals are missing aliases")
end)

test("cache.invalidate", {}, function()
	local container = Instance.new("Folder")
	local part = Instance.new("Part", container)
	cache.invalidate(container:FindFirstChild("Part"))
	assert(part ~= container:FindFirstChild("Part"), "Reference `part` could not be invalidated")
end)

test("cache.iscached", {}, function()
	local part = Instance.new("Part")
	assert(cache.iscached(part), "Part should be cached")
	cache.invalidate(part)
	assert(not cache.iscached(part), "Part should not be cached")
end)

test("cache.replace", {}, function()
	local part = Instance.new("Part")
	local fire = Instance.new("Fire")
	cache.replace(part, fire)
	assert(part ~= fire, "Part was not replaced with Fire")
end)

test("cloneref", {}, function()
	local part = Instance.new("Part")
	local clone = cloneref(part)
	assert(part ~= clone, "Clone should not be equal to original")
	clone.Name = "Test"
	assert(part.Name == "Test", "Clone should have updated the original")
end)

test("compareinstances", {}, function()
	local part = Instance.new("Part")
	local clone = cloneref(part)
	assert(part ~= clone, "Clone should not be equal to original")
	assert(compareinstances(part, clone), "Clone should be equal to original when using compareinstances()")
end)

local function shallowEqual(t1, t2)
	if t1 == t2 then
		return true
	end

	local UNIQUE_TYPES = {
		["function"] = true,
		["table"] = true,
		["userdata"] = true,
		["thread"] = true,
	}

	for k, v in pairs(t1) do
		if UNIQUE_TYPES[type(v)] then
			if type(t2[k]) ~= type(v) then
				return false
			end
		elseif t2[k] ~= v then
			return false
		end
	end

	for k, v in pairs(t2) do
		if UNIQUE_TYPES[type(v)] then
			if type(t2[k]) ~= type(v) then
				return false
			end
		elseif t1[k] ~= v then
			return false
		end
	end

	return true
end

test("checkcaller", {}, function()
	assert(checkcaller(), "Main scope should return true")
end)

test("clonefunction", {}, function()
	local function test()
		return "success"
	end
	local copy = clonefunction(test)
	assert(test() == copy(), "The clone should return the same value as the original")
	assert(test ~= copy, "The clone should not be equal to the original")
end)

test("getcallingscript", {})

test("getscriptclosure", {"getscriptfunction"}, function()
	local module = game:GetService("CoreGui").RobloxGui.Modules.Common.Constants
	local constants = getrenv().require(module)
	local generated = getscriptclosure(module)()
	assert(constants ~= generated, "Generated module should not match the original")
	assert(shallowEqual(constants, generated), "Generated constant table should be shallow equal to the original")
end)

test("hookfunction", {"replaceclosure"}, function()
	local function test()
		return true
	end
	local ref = hookfunction(test, function()
		return false
	end)
	assert(test() == false, "Function should return false")
	assert(ref() == true, "Original function should return true")
	assert(test ~= ref, "Original function should not be same as the reference")
end)

test("iscclosure", {}, function()
	assert(iscclosure(print) == true, "Function 'print' should be a C closure")
	assert(iscclosure(function() end) == false, "Executor function should not be a C closure")
end)

test("islclosure", {}, function()
	assert(islclosure(print) == false, "Function 'print' should not be a Lua closure")
	assert(islclosure(function() end) == true, "Executor function should be a Lua closure")
end)

test("isexecutorclosure", {"checkclosure", "isourclosure"}, function()
	assert(isexecutorclosure(isexecutorclosure) == true, "Did not return true for an executor global")
	assert(isexecutorclosure(newcclosure(function() end)) == true, "Did not return true for an executor C closure")
	assert(isexecutorclosure(function() end) == true, "Did not return true for an executor Luau closure")
	assert(isexecutorclosure(print) == false, "Did not return false for a Roblox global")
end)

test("loadstring", {}, function()
	local animate = game:GetService("Players").LocalPlayer.Character.Animate
	local bytecode = getscriptbytecode(animate)
	local func = loadstring(bytecode)
	assert(type(func) ~= "function", "Luau bytecode should not be loadable!")
	assert(assert(loadstring("return ... + 1"))(1) == 2, "Failed to do simple math")
	assert(type(select(2, loadstring("f"))) == "string", "Loadstring did not return anything for a compiler error")
end)

test("newcclosure", {}, function()
	local function test()
		return true
	end
	local testC = newcclosure(test)
	assert(test() == testC(), "New C closure should return the same value as the original")
	assert(test ~= testC, "New C closure should not be same as the original")
	assert(iscclosure(testC), "New C closure should be a C closure")
end)

test("rconsoleclear", {"consoleclear"})

test("rconsolecreate", {"consolecreate"})

test("rconsoledestroy", {"consoledestroy"})

test("rconsoleinput", {"consoleinput"})

test("rconsoleprint", {"consoleprint"})

test("rconsolesettitle", {"rconsolename", "consolesettitle"})

test("crypt.base64encode", {"crypt.base64.encode", "crypt.base64_encode", "base64.encode", "base64_encode"}, function()
	assert(crypt.base64encode("test") == "dGVzdA==", "Base64 encoding failed")
end)

test("crypt.base64decode", {"crypt.base64.decode", "crypt.base64_decode", "base64.decode", "base64_decode"}, function()
	assert(crypt.base64decode("dGVzdA==") == "test", "Base64 decoding failed")
end)

test("crypt.encrypt", {}, function()
	local key = crypt.generatekey()
	local encrypted, iv = crypt.encrypt("test", key, nil, "CBC")
	assert(iv, "crypt.encrypt should return an IV")
	local decrypted = crypt.decrypt(encrypted, key, iv, "CBC")
	assert(decrypted == "test", "Failed to decrypt raw string from encrypted data")
end)

test("crypt.decrypt", {}, function()
	local key, iv = crypt.generatekey(), crypt.generatekey()
	local encrypted = crypt.encrypt("test", key, iv, "CBC")
	local decrypted = crypt.decrypt(encrypted, key, iv, "CBC")
	assert(decrypted == "test", "Failed to decrypt raw string from encrypted data")
end)

test("crypt.generatebytes", {}, function()
	local size = math.random(10, 100)
	local bytes = crypt.generatebytes(size)
	assert(#crypt.base64decode(bytes) == size, "The decoded result should be " .. size .. " bytes long (got " .. #crypt.base64decode(bytes) .. " decoded, " .. #bytes .. " raw)")
end)

test("crypt.generatekey", {}, function()
	local key = crypt.generatekey()
	assert(#crypt.base64decode(key) == 32, "Generated key should be 32 bytes long when decoded")
end)

test("crypt.hash", {}, function()
	local algorithms = {'sha1', 'sha384', 'sha512', 'md5', 'sha256', 'sha3-224', 'sha3-256', 'sha3-512'}
	for _, algorithm in ipairs(algorithms) do
		local hash = crypt.hash("test", algorithm)
		assert(hash, "crypt.hash on algorithm '" .. algorithm .. "' should return a hash")
	end
end)

test("debug.getconstant", {}, function()
	local function test()
		print("Hello, world!")
	end
	assert(debug.getconstant(test, 1) == "print", "First constant must be print")
	assert(debug.getconstant(test, 2) == nil, "Second constant must be nil")
	assert(debug.getconstant(test, 3) == "Hello, world!", "Third constant must be 'Hello, world!'")
end)

test("debug.getconstants", {}, function()
	local function test()
		local num = 5000 .. 50000
		print("Hello, world!", num, warn)
	end
	local constants = debug.getconstants(test)
	assert(constants[1] == 50000, "First constant must be 50000")
	assert(constants[2] == "print", "Second constant must be print")
	assert(constants[3] == nil, "Third constant must be nil")
	assert(constants[4] == "Hello, world!", "Fourth constant must be 'Hello, world!'")
	assert(constants[5] == "warn", "Fifth constant must be warn")
end)

test("debug.getinfo", {}, function()
	local types = {
		source = "string",
		short_src = "string",
		func = "function",
		what = "string",
		currentline = "number",
		name = "string",
		nups = "number",
		numparams = "number",
		is_vararg = "number",
	}
	local function test(...)
		print(...)
	end
	local info = debug.getinfo(test)
	for k, v in pairs(types) do
		assert(info[k] ~= nil, "Did not return a table with a '" .. k .. "' field")
		assert(type(info[k]) == v, "Did not return a table with " .. k .. " as a " .. v .. " (got " .. type(info[k]) .. ")")
	end
end)

test("debug.getproto", {}, function()
	local function test()
		local function proto()
			return true
		end
	end
	local proto = debug.getproto(test, 1, true)[1]
	local realproto = debug.getproto(test, 1)
	assert(proto, "Failed to get the inner function")
	assert(proto() == true, "The inner function did not return anything")
	if not realproto() then
		return "Proto return values are disabled on this executor"
	end
end)

test("debug.getprotos", {}, function()
	local function test()
		local function _1()
			return true
		end
		local function _2()
			return true
		end
		local function _3()
			return true
		end
	end
	for i in ipairs(debug.getprotos(test)) do
		local proto = debug.getproto(test, i, true)[1]
		local realproto = debug.getproto(test, i)
		assert(proto(), "Failed to get inner function " .. i)
		if not realproto() then
			return "Proto return values are disabled on this executor"
		end
	end
end)

test("debug.getstack", {}, function()
	local _ = "a" .. "b"
	assert(debug.getstack(1, 1) == "ab", "The first item in the stack should be 'ab'")
	assert(debug.getstack(1)[1] == "ab", "The first item in the stack table should be 'ab'")
end)

test("debug.getupvalue", {}, function()
	local upvalue = function() end
	local function test()
		print(upvalue)
	end
	assert(debug.getupvalue(test, 1) == upvalue, "Unexpected value returned from debug.getupvalue")
end)

test("debug.getupvalues", {}, function()
	local upvalue = function() end
	local function test()
		print(upvalue)
	end
	local upvalues = debug.getupvalues(test)
	assert(upvalues[1] == upvalue, "Unexpected value returned from debug.getupvalues")
end)

test("debug.setconstant", {}, function()
	local function test()
		return "fail"
	end
	debug.setconstant(test, 1, "success")
	assert(test() == "success", "debug.setconstant did not set the first constant")
end)

test("debug.setstack", {}, function()
	local function test()
		return "fail", debug.setstack(1, 1, "success")
	end
	assert(test() == "success", "debug.setstack did not set the first stack item")
end)

test("debug.setupvalue", {}, function()
	local function upvalue()
		return "fail"
	end
	local function test()
		return upvalue()
	end
	debug.setupvalue(test, 1, function()
		return "success"
	end)
	assert(test() == "success", "debug.setupvalue did not set the first upvalue")
end)

)LUA"
R"LUA(

if isfolder and makefolder and delfolder then
	if isfolder(".tests") then
		delfolder(".tests")
	end
	makefolder(".tests")
end

test("readfile", {}, function()
	writefile(".tests/readfile.txt", "success")
	assert(readfile(".tests/readfile.txt") == "success", "Did not return the contents of the file")
end)

test("listfiles", {}, function()
	makefolder(".tests/listfiles")
	writefile(".tests/listfiles/test_1.txt", "success")
	writefile(".tests/listfiles/test_2.txt", "success")
	local files = listfiles(".tests/listfiles")
	assert(#files == 2, "Did not return the correct number of files")
	assert(isfile(files[1]), "Did not return a file path")
	assert(readfile(files[1]) == "success", "Did not return the correct files")
	makefolder(".tests/listfiles_2")
	makefolder(".tests/listfiles_2/test_1")
	makefolder(".tests/listfiles_2/test_2")
	local folders = listfiles(".tests/listfiles_2")
	assert(#folders == 2, "Did not return the correct number of folders")
	assert(isfolder(folders[1]), "Did not return a folder path")
end)

test("writefile", {}, function()
	writefile(".tests/writefile.txt", "success")
	assert(readfile(".tests/writefile.txt") == "success", "Did not write the file")
	local requiresFileExt = pcall(function()
		writefile(".tests/writefile", "success")
		assert(isfile(".tests/writefile.txt"))
	end)
	if not requiresFileExt then
		return "This executor requires a file extension in writefile"
	end
end)

test("makefolder", {}, function()
	makefolder(".tests/makefolder")
	assert(isfolder(".tests/makefolder"), "Did not create the folder")
end)

test("appendfile", {}, function()
	writefile(".tests/appendfile.txt", "su")
	appendfile(".tests/appendfile.txt", "cce")
	appendfile(".tests/appendfile.txt", "ss")
	assert(readfile(".tests/appendfile.txt") == "success", "Did not append the file")
end)

test("isfile", {}, function()
	writefile(".tests/isfile.txt", "success")
	assert(isfile(".tests/isfile.txt") == true, "Did not return true for a file")
	assert(isfile(".tests") == false, "Did not return false for a folder")
	assert(isfile(".tests/doesnotexist.exe") == false, "Did not return false for a nonexistent path (got " .. tostring(isfile(".tests/doesnotexist.exe")) .. ")")
end)

test("isfolder", {}, function()
	assert(isfolder(".tests") == true, "Did not return false for a folder")
	assert(isfolder(".tests/doesnotexist.exe") == false, "Did not return false for a nonexistent path (got " .. tostring(isfolder(".tests/doesnotexist.exe")) .. ")")
end)

test("delfolder", {}, function()
	makefolder(".tests/delfolder")
	delfolder(".tests/delfolder")
	assert(isfolder(".tests/delfolder") == false, "Failed to delete folder (isfolder = " .. tostring(isfolder(".tests/delfolder")) .. ")")
end)

test("delfile", {}, function()
	writefile(".tests/delfile.txt", "Hello, world!")
	delfile(".tests/delfile.txt")
	assert(isfile(".tests/delfile.txt") == false, "Failed to delete file (isfile = " .. tostring(isfile(".tests/delfile.txt")) .. ")")
end)

test("loadfile", {}, function()
	writefile(".tests/loadfile.txt", "return ... + 1")
	assert(assert(loadfile(".tests/loadfile.txt"))(1) == 2, "Failed to load a file with arguments")
	writefile(".tests/loadfile.txt", "f")
	local callback, err = loadfile(".tests/loadfile.txt")
	assert(err and not callback, "Did not return an error message for a compiler error")
end)

test("dofile", {})

test("isrbxactive", {"isgameactive"}, function()
	assert(type(isrbxactive()) == "boolean", "Did not return a boolean value")
end)

test("mouse1click", {})

test("mouse1press", {})

test("mouse1release", {})

test("mouse2click", {})

test("mouse2press", {})

test("mouse2release", {})

test("mousemoveabs", {})

test("mousemoverel", {})

test("mousescroll", {})

test("fireclickdetector", {}, function()
	local detector = Instance.new("ClickDetector")
	fireclickdetector(detector, 50, "MouseHoverEnter")
end)

test("getcallbackvalue", {}, function()
	local bindable = Instance.new("BindableFunction")
	local function test()
	end
	bindable.OnInvoke = test
	assert(getcallbackvalue(bindable, "OnInvoke") == test, "Did not return the correct value")
end)

test("getconnections", {}, function()
	local types = {
		Enabled = "boolean",
		ForeignState = "boolean",
		LuaConnection = "boolean",
		Function = "function",
		Thread = "thread",
		Fire = "function",
		Defer = "function",
		Disconnect = "function",
		Disable = "function",
		Enable = "function",
	}
	local bindable = Instance.new("BindableEvent")
	bindable.Event:Connect(function() end)
	local connection = getconnections(bindable.Event)[1]
	for k, v in pairs(types) do
		assert(connection[k] ~= nil, "Did not return a table with a '" .. k .. "' field")
		assert(type(connection[k]) == v, "Did not return a table with " .. k .. " as a " .. v .. " (got " .. type(connection[k]) .. ")")
	end
end)

test("getcustomasset", {}, function()
	writefile(".tests/getcustomasset.txt", "success")
	local contentId = getcustomasset(".tests/getcustomasset.txt")
	assert(type(contentId) == "string", "Did not return a string")
	assert(#contentId > 0, "Returned an empty string")
	assert(string.match(contentId, "rbxasset://"))
end)

test("gethiddenproperty", {}, function()
	local fire = Instance.new("Fire")
	local property, isHidden = gethiddenproperty(fire, "size_xml")
	assert(property == 5, "Did not return the correct value")
	assert(isHidden == true, "Did not return whether the property was hidden")
end)

test("sethiddenproperty", {}, function()
	local fire = Instance.new("Fire")
	local hidden = sethiddenproperty(fire, "size_xml", 10)
	assert(hidden, "Did not return true for the hidden property")
	assert(gethiddenproperty(fire, "size_xml") == 10, "Did not set the hidden property")
end)

test("gethui", {}, function()
	assert(typeof(gethui()) == "Instance", "Did not return an Instance")
end)

test("getinstances", {}, function()
	assert(getinstances()[1]:IsA("Instance"), "The first value is not an Instance")
end)

test("getnilinstances", {}, function()
	assert(getnilinstances()[1]:IsA("Instance"), "The first value is not an Instance")
	assert(getnilinstances()[1].Parent == nil, "The first value is not parented to nil")
end)

test("isscriptable", {}, function()
	local fire = Instance.new("Fire")
	assert(isscriptable(fire, "size_xml") == false, "Did not return false for a non-scriptable property (size_xml)")
	assert(isscriptable(fire, "Size") == true, "Did not return true for a scriptable property (Size)")
end)

test("setscriptable", {}, function()
	local fire = Instance.new("Fire")
	local wasScriptable = setscriptable(fire, "size_xml", true)
	assert(wasScriptable == false, "Did not return false for a non-scriptable property (size_xml)")
	assert(isscriptable(fire, "size_xml") == true, "Did not set the scriptable property")
	fire = Instance.new("Fire")
	assert(isscriptable(fire, "size_xml") == false, "⚠️⚠️ setscriptable persists between unique instances ⚠️⚠️")
end)

test("setrbxclipboard", {})

-- Metatable

test("getrawmetatable", {}, function()
	local metatable = { __metatable = "Locked!" }
	local object = setmetatable({}, metatable)
	assert(getrawmetatable(object) == metatable, "Did not return the metatable")
end)

test("hookmetamethod", {}, function()
	local object = setmetatable({}, { __index = newcclosure(function() return false end), __metatable = "Locked!" })
	local ref = hookmetamethod(object, "__index", function() return true end)
	assert(object.test == true, "Failed to hook a metamethod and change the return value")
	assert(ref() == false, "Did not return the original function")
end)

test("getnamecallmethod", {}, function()
	local method
	local ref
	ref = hookmetamethod(game, "__namecall", function(...)
		if not method then
			method = getnamecallmethod()
		end
		return ref(...)
	end)
	game:GetService("Lighting")
	assert(method == "GetService", "Did not get the correct method (GetService)")
end)

test("isreadonly", {}, function()
	local object = {}
	table.freeze(object)
	assert(isreadonly(object), "Did not return true for a read-only table")
end)

test("setrawmetatable", {}, function()
	local object = setmetatable({}, { __index = function() return false end, __metatable = "Locked!" })
	local objectReturned = setrawmetatable(object, { __index = function() return true end })
	assert(object, "Did not return the original object")
	assert(object.test == true, "Failed to change the metatable")
	if objectReturned then
		return objectReturned == object and "Returned the original object" or "Did not return the original object"
	end
end)

test("setreadonly", {}, function()
	local object = { success = false }
	table.freeze(object)
	setreadonly(object, false)
	object.success = true
	assert(object.success, "Did not allow the table to be modified")
end)

-- Miscellaneous

test("identifyexecutor", {"getexecutorname"}, function()
	local name, version = identifyexecutor()
	assert(type(name) == "string", "Did not return a string for the name")
	return type(version) == "string" and "Returns version as a string" or "Does not return version"
end)

test("lz4compress", {}, function()
	local raw = "Hello, world!"
	local compressed = lz4compress(raw)
	assert(type(compressed) == "string", "Compression did not return a string")
	assert(lz4decompress(compressed, #raw) == raw, "Decompression did not return the original string")
end)

test("lz4decompress", {}, function()
	local raw = "Hello, world!"
	local compressed = lz4compress(raw)
	assert(type(compressed) == "string", "Compression did not return a string")
	assert(lz4decompress(compressed, #raw) == raw, "Decompression did not return the original string")
end)

test("messagebox", {})

test("queue_on_teleport", {"queueonteleport"})

test("request", {"http.request", "http_request"}, function()
	local response = request({
		Url = "https://httpbin.org/user-agent",
		Method = "GET",
	})
	assert(type(response) == "table", "Response must be a table")
	assert(response.StatusCode == 200, "Did not return a 200 status code")
	local data = game:GetService("HttpService"):JSONDecode(response.Body)
	assert(type(data) == "table" and type(data["user-agent"]) == "string", "Did not return a table with a user-agent key")
	return "User-Agent: " .. data["user-agent"]
end)

test("setclipboard", {"toclipboard"})

test("setfpscap", {}, function()
	local renderStepped = game:GetService("RunService").RenderStepped
	local function step()
		renderStepped:Wait()
		local sum = 0
		for _ = 1, 5 do
			sum += 1 / renderStepped:Wait()
		end
		return math.round(sum / 5)
	end
	setfpscap(60)
	local step60 = step()
	setfpscap(0)
	local step0 = step()
	return step60 .. "fps @60 • " .. step0 .. "fps @0"
end)

)LUA"
R"LUA(

test("getgc", {}, function()
	local gc = getgc()
	assert(type(gc) == "table", "Did not return a table")
	assert(#gc > 0, "Did not return a table with any values")
end)

test("getgenv", {}, function()
	getgenv().__TEST_GLOBAL = true
	assert(__TEST_GLOBAL, "Failed to set a global variable")
	getgenv().__TEST_GLOBAL = nil
end)

test("getloadedmodules", {}, function()
	local modules = getloadedmodules()
	assert(type(modules) == "table", "Did not return a table")
	assert(#modules > 0, "Did not return a table with any values")
	assert(typeof(modules[1]) == "Instance", "First value is not an Instance")
	assert(modules[1]:IsA("ModuleScript"), "First value is not a ModuleScript")
end)

test("getrenv", {}, function()
	assert(_G ~= getrenv()._G, "The variable _G in the executor is identical to _G in the game")
end)

test("getrunningscripts", {}, function()
	local scripts = getrunningscripts()
	assert(type(scripts) == "table", "Did not return a table")
	assert(#scripts > 0, "Did not return a table with any values")
	assert(typeof(scripts[1]) == "Instance", "First value is not an Instance")
	assert(scripts[1]:IsA("ModuleScript") or scripts[1]:IsA("LocalScript"), "First value is not a ModuleScript or LocalScript")
end)

test("getscriptbytecode", {"dumpstring"}, function()
	local animate = game:GetService("Players").LocalPlayer.Character.Animate
	local bytecode = getscriptbytecode(animate)
	assert(type(bytecode) == "string", "Did not return a string for Character.Animate (a " .. animate.ClassName .. ")")
end)

test("getscripthash", {}, function()
	local animate = game:GetService("Players").LocalPlayer.Character.Animate:Clone()
	local hash = getscripthash(animate)
	local source = animate.Source
	animate.Source = "print('Hello, world!')"
	task.defer(function()
		animate.Source = source
	end)
	local newHash = getscripthash(animate)
	assert(hash ~= newHash, "Did not return a different hash for a modified script")
	assert(newHash == getscripthash(animate), "Did not return the same hash for a script with the same source")
end)

test("getscripts", {}, function()
	local scripts = getscripts()
	assert(type(scripts) == "table", "Did not return a table")
	assert(#scripts > 0, "Did not return a table with any values")
	assert(typeof(scripts[1]) == "Instance", "First value is not an Instance")
	assert(scripts[1]:IsA("ModuleScript") or scripts[1]:IsA("LocalScript"), "First value is not a ModuleScript or LocalScript")
end)

test("getsenv", {}, function()
	local animate = game:GetService("Players").LocalPlayer.Character.Animate
	local env = getsenv(animate)
	assert(type(env) == "table", "Did not return a table for Character.Animate (a " .. animate.ClassName .. ")")
	assert(env.script == animate, "The script global is not identical to Character.Animate")
end)

test("getthreadidentity", {"getidentity", "getthreadcontext"}, function()
	assert(type(getthreadidentity()) == "number", "Did not return a number")
end)

test("setthreadidentity", {"setidentity", "setthreadcontext"}, function()
	setthreadidentity(3)
	assert(getthreadidentity() == 3, "Did not set the thread identity")
end)

test("Drawing", {})

test("Drawing.new", {}, function()
	local drawing = Drawing.new("Square")
	drawing.Visible = false
	local canDestroy = pcall(function()
		drawing:Destroy()
	end)
	assert(canDestroy, "Drawing:Destroy() should not throw an error")
end)

test("Drawing.Fonts", {}, function()
	assert(Drawing.Fonts.UI == 0, "Did not return the correct id for UI")
	assert(Drawing.Fonts.System == 1, "Did not return the correct id for System")
	assert(Drawing.Fonts.Plex == 2, "Did not return the correct id for Plex")
	assert(Drawing.Fonts.Monospace == 3, "Did not return the correct id for Monospace")
end)

test("isrenderobj", {}, function()
	local drawing = Drawing.new("Image")
	drawing.Visible = true
	assert(isrenderobj(drawing) == true, "Did not return true for an Image")
	assert(isrenderobj(newproxy()) == false, "Did not return false for a blank table")
end)

test("getrenderproperty", {}, function()
	local drawing = Drawing.new("Image")
	drawing.Visible = true
	assert(type(getrenderproperty(drawing, "Visible")) == "boolean", "Did not return a boolean value for Image.Visible")
	local success, result = pcall(function()
		return getrenderproperty(drawing, "Color")
	end)
	if not success or not result then
		return "Image.Color is not supported"
	end
end)

test("setrenderproperty", {}, function()
	local drawing = Drawing.new("Square")
	drawing.Visible = true
	setrenderproperty(drawing, "Visible", false)
	assert(drawing.Visible == false, "Did not set the value for Square.Visible")
end)

test("cleardrawcache", {}, function()
	cleardrawcache()
end)

test("WebSocket", {})

test("WebSocket.connect", {}, function()
	local types = {
		Send = "function",
		Close = "function",
		OnMessage = {"table", "userdata"},
		OnClose = {"table", "userdata"},
	}
	local ws = WebSocket.connect("ws://echo.websocket.events")
	assert(type(ws) == "table" or type(ws) == "userdata", "Did not return a table or userdata")
	for k, v in pairs(types) do
		if type(v) == "table" then
			assert(table.find(v, type(ws[k])), "Did not return a " .. table.concat(v, ", ") .. " for " .. k .. " (a " .. type(ws[k]) .. ")")
		else
			assert(type(ws[k]) == v, "Did not return a " .. v .. " for " .. k .. " (a " .. type(ws[k]) .. ")")
		end
	end
	ws:Close()
end)
)LUA";

}
