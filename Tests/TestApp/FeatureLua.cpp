
#include "FeatureLua.h"

#include "Session.h"

using namespace Phoenix;

void FeatureLua::Initialize()
{
    IFeature::Initialize();

    BlockBuffer* buffer = Session->GetBuffer();
    FeatureLuaDynamicBlock* dynamicBlock = buffer->GetBlock<FeatureLuaDynamicBlock>();
    if (!dynamicBlock)
    {
        return;
    }

    sol::state& lua = dynamicBlock->State;

    lua.open_libraries(sol::lib::base);

    std::string fileName = "C:\\Users\\jlfar\\OneDrive\\Documents\\Unreal Projects\\PhoenixSim\\Tests\\TestApp\\Maps\\test_script.lua";
    sol::load_result result = lua.load_file(fileName);

    sol::load_status status = result.status();

    if (status != sol::load_status::ok)
    {
        
    }
}

void FeatureLua::Shutdown()
{
    IFeature::Shutdown();
}

void FeatureLua::OnUpdate(const FeatureUpdateArgs& args)
{
    IFeature::OnUpdate(args);

    BlockBuffer* buffer = Session->GetBuffer();
    FeatureLuaDynamicBlock* dynamicBlock = buffer->GetBlock<FeatureLuaDynamicBlock>();
    if (!dynamicBlock)
    {
        return;
    }

    sol::state& lua = dynamicBlock->State;

    sol::function luaUpdateFunc = lua["update"];
    std::function<void()> updateFunction = luaUpdateFunc;

    updateFunction();
}
