print("Made by FlyNesty Team")
print("Join our discord server and visit site flynesty.xyz")
print("GitHub Injector Repo: https://github.com/ImHartash/FlyNestyInjectorNew")

local _fetch_stubmodule do
	local current_module = 1
	local modules_list = {}
	local in_use_modules = {}

	for _, obj in game:FindService("CoreGui").RobloxGui.Modules:GetDescendants() do
		if not obj:IsA("ModuleScript") then
			if obj.Name:match("AvatarExperience") then
				for _, o in obj:GetDescendants() do
					if o.Name == "Flags" then
						for _, oa in o:GetDescendants() do
							if not oa:IsA("ModuleScript") then continue end
							table.insert(modules_list, oa:Clone())
						end
					elseif o.Name == "Test" then
						for _, oa in o:GetDescendants() do
							if not oa:IsA("ModuleScript") then continue end
							table.insert(modules_list, oa:Clone())
						end
					end
				end
			else
				if 
				obj.Name:match("ReportAnything") or obj.Name:match("TestHelpers")
				then
					for _, o in obj:GetDescendants() do
						if not o:IsA("ModuleScript") then continue end
						table.insert(modules_list, o:Clone())
					end
				end
			end
				
			continue 
		end
	end

	local function find_new_module( ... )
		local random_module = math.random(1, #modules_list)
		while random_module == current_module or in_use_modules[random_module] do
			random_module = math.random(1, #modules_list)
		end
		return random_module
	end

	function _fetch_stubmodule( ... )
		local random_module = find_new_module()

		in_use_modules[current_module] = nil
		current_module = random_module
		in_use_modules[current_module] = true

		return modules_list[random_module]
	end
end

local fetch_stubmodule = _fetch_stubmodule
local bridge = table.create(0)

function bridge:post(action, data) 
	local success, result = pcall(function()
		local URL = "http://localhost:6969/bridge?action=" .. action
		local RequestBody = HttpService:JSONEncode(data)
		local Params = {
			Url = url,
			Method = "POST",
			Body = RequestBody,
			Headers = {
				["Content-Type"] = "application/json"
			}
		}

		local request = HttpService:RequestInternal(Params)

		local response = nil

		request:Start(function(success, result)
			if success then
				response = result
			else
				response = {}
			end
		end)

		while response == nil do
			task.wait()
		end

		if response.StatusMessage == "OK" then
			return HttpService:JSONDecode(response.Body)
		end
	end)

	if not success then
		return "ERROR: " .. tostring(result)
	end
	return result
end

function bridge:request(data, from)
	local success, res = pcall(function()
		if data.Method then data.Method = data.Method:upper() end
		local heads
		for t, v in data do
			if t:lower() == "headers" then
				heads = v
				break
			end
		end
	
		if not heads then
			heads = table.create(0)
		end
	
		data.Headers = heads
		data.Headers["Roblox-Place-Id"] = tostring(game.PlaceId)
		data.Headers["Roblox-Game-Id"] = tostring(game.JobId)
		data.Headers["Roblox-Session-Id"] = HttpService:JSONEncode({
			["GameId"] = tostring(game.JobId),
			["PlaceId"] = tostring(game.PlaceId)
		})
	
		if from then
			data.Headers["User-Agent"] = "Roblox/WinInet"
		end
	
		local url = "http://localhost:1337/bridge?action=request"
	
		local request = HttpService:RequestInternal({
			Url = url,
			Method = "POST",
			Body = HttpService:JSONEncode({
				source=source,
				type="request",
				body=HttpService:JSONEncode(data)
			}),
			Headers = {
				["Content-Type"] = "application/json",
			}
		})
	
		local response = nil

		request:Start(function(success, result)
			if success then
				response = result
			else
				response = {}
			end
		end)

		while (response == nil) do 
			task.wait() 
		end

		return HttpService:JSONDecode(response.Body)
	end)
	if not success then
		return "ERROR: " .. tostring(res)
	else
		return res
	end
end

local RunService = game:GetService("RunService")
if script.Name == "JestGlobals" then
	local exec = Instance.new("BoolValue")
	exec.Name = "Exec"
	exec.Parent = script

	local holder = Instance.new("ObjectValue")
	holder.Parent = script
	holder.Name = "Holder"
	holder.Value = fetch_stubmodule():Clone()

	local ls_exec = Instance.new("BoolValue")
	ls_exec.Name = "Loadstring"
	ls_exec.Parent = script

	local ls_holder = Instance.new("ObjectValue")
	ls_holder.Parent = script
	ls_holder.Name = "LoadstringHolder"
	ls_holder.Value = fetch_stubmodule():Clone()

	local justcalledlocal = false

	local exec = script.Exec
	local holder = script.Holder
	local ls_bool = script.Loadstring

	task.spawn(function( ... )
		RunService.RenderStepped:Connect(function()
			local current_time = tick()
			if exec.Value == true then
				if holder.Value == nil then
					game:GetService("StarterGui"):SetCore("SendNotification", {
						Title = "FlyNesty Injector",
						Text = "Something went wrong. Please try execute script again. flynesty.xyz",
						Icon = ""
					})
					holder.Value = fetch_stubmodule():Clone()
				end

				local s, func = pcall(require, holder.Value)

				holder.Value = fetch_stubmodule():Clone()
				exec.Value = false
				if s and type(func) == "function" then
					func()
				end
			end
		end)
	end)

	loadstring = (function(source, chunkname)
		if chunkname == nil then chunkname = "flynesty" end
		if chunkname == "" then chunkname = "flynesty" end

		while ls_bool do
			task.wait()
		end

		ls_bool = true

		bridge:post("loadstring", {source=source, type="loadstring"})

		local script_load = ls_holder.Value
		local original = script_load.Name
		script_load.Name = chunkname

		local s, func = pcall(require, script_load)
		script_load.Name = original

		ls_bool = false

		local new_module = fetch_stubmodule():Clone()
		ls_holder.Value = new_module
		script_load:Destroy()

		if s and type(func) == "function" then
			setfenv(func, setmetatable({}, {__index = getfenv()}))
			return func
		end
	end)
end

wait()

if script.Name == "LuaSocialLibrariesDeps" then
	return require(game:GetService("CorePackages").Packages.LuaSocialLibrariesDeps)
end
if script.Name == "JestGlobals" then
	return require(script)
end
if script.Name == "Url" then
	local a={}
	local b=game:GetService("ContentProvider")
	local function c(d)
		local e,f=d:find("%.")
		local g=d:sub(f+1)
		if g:sub(-1)~="/"then
			g=g.."/"
		end;
		return g
	end;
	local d=b.BaseUrl
	local g=c(d)
	local h=string.format("https://games.%s",g)
	local i=string.format("https://apis.rcs.%s",g)
	local j=string.format("https://apis.%s",g)
	local k=string.format("https://accountsettings.%s",g)
	local l=string.format("https://gameinternationalization.%s",g)
	local m=string.format("https://locale.%s",g)
	local n=string.format("https://users.%s",g)
	local o={GAME_URL=h,RCS_URL=i,APIS_URL=j,ACCOUNT_SETTINGS_URL=k,GAME_INTERNATIONALIZATION_URL=l,LOCALE_URL=m,ROLES_URL=n}setmetatable(a,{__newindex=function(p,q,r)end,__index=function(p,r)return o[r]end})
	return a
end
while wait(9e9) do wait(9e9);end