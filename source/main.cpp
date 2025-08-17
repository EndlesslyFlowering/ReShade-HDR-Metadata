
#include <stdio.h>
#include <cstdint>
#include <basetsd.h>
#include <string>
#include <dxgi1_5.h>
#include <cmath>
#include <mutex>

#include <dxgi1_6.h>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>
#include <winnt.h>


static const double DEFAULT_PRIMARY_RED[2]   = { 0.708,  0.292 };
static const double DEFAULT_PRIMARY_GREEN[2] = { 0.170,  0.797 };
static const double DEFAULT_PRIMARY_BLUE[2]  = { 0.131,  0.046 };

static const double DEFAULT_WHITE_POINT[2]   = { 0.3127, 0.3290 };

static const int32_t DEFAULT_MAX_MDL  = 1000;
static const float   DEFAULT_MIN_MDL  =    0.f;
static const int32_t DEFAULT_MAX_CLL  = 1000;
static const int32_t DEFAULT_MAX_FALL =  100;

struct HDR_Metadata
{
  //primaries
  double primary_red[2]   = { DEFAULT_PRIMARY_RED[0],   DEFAULT_PRIMARY_RED[1]   };
  double primary_green[2] = { DEFAULT_PRIMARY_GREEN[0], DEFAULT_PRIMARY_GREEN[1] };
  double primary_blue[2]  = { DEFAULT_PRIMARY_BLUE[0],  DEFAULT_PRIMARY_BLUE[1]  };

  //white point
  double white_point[2] = { DEFAULT_WHITE_POINT[0], DEFAULT_WHITE_POINT[1] };

  //max mastering display luminance
  int32_t max_mdl  = DEFAULT_MAX_MDL;
  //min mastering display luminance
  float   min_mdl  = DEFAULT_MIN_MDL;
  //max content light level
  int32_t max_cll  = DEFAULT_MAX_CLL;
  //max frame average light level
  int32_t max_fall = DEFAULT_MAX_FALL;
};


static struct Global_Struct
{
  std::mutex mutex;

  reshade::api::device*    device    = nullptr;
  reshade::api::swapchain* swapchain = nullptr;

  bool api_is_supported = false;

  HDR_Metadata hdr_metadata;

  std::string sethdrmetadata_message = "";
  ImVec4      sethdrmetadata_message_colour = ImVec4(1.f, 1.f, 1.f, 1.f);
} Global;

DXGI_HDR_METADATA_HDR10 Get_DXGI_HDR10_Metadata()
{
  DXGI_HDR_METADATA_HDR10 ret;

  ret.RedPrimary[0]   = static_cast<UINT16>(std::round(Global.hdr_metadata.primary_red[0]   * 50000.0));
  ret.RedPrimary[1]   = static_cast<UINT16>(std::round(Global.hdr_metadata.primary_red[1]   * 50000.0));
  ret.GreenPrimary[0] = static_cast<UINT16>(std::round(Global.hdr_metadata.primary_green[0] * 50000.0));
  ret.GreenPrimary[1] = static_cast<UINT16>(std::round(Global.hdr_metadata.primary_green[1] * 50000.0));
  ret.BluePrimary[0]  = static_cast<UINT16>(std::round(Global.hdr_metadata.primary_blue[0]  * 50000.0));
  ret.BluePrimary[1]  = static_cast<UINT16>(std::round(Global.hdr_metadata.primary_blue[1]  * 50000.0));
  ret.WhitePoint[0]   = static_cast<UINT16>(std::round(Global.hdr_metadata.white_point[0]   * 50000.0));
  ret.WhitePoint[1]   = static_cast<UINT16>(std::round(Global.hdr_metadata.white_point[1]   * 50000.0));

  ret.MaxMasteringLuminance = static_cast<UINT>(Global.hdr_metadata.max_mdl);
  ret.MinMasteringLuminance = static_cast<UINT>(std::roundf(Global.hdr_metadata.min_mdl * 10000.f));

  ret.MaxContentLightLevel      = static_cast<UINT16>(Global.hdr_metadata.max_cll);
  ret.MaxFrameAverageLightLevel = static_cast<UINT16>(Global.hdr_metadata.max_fall);

  return ret;
}


static void Draw_UI(reshade::api::effect_runtime* runtime)
{
  if (Global.api_is_supported)
  {
    bool modified = false;

    if (ImGui::BeginTable("table for CLL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
      //header
      //ImGui::TableSetupColumn("");
      //ImGui::TableSetupColumn("");
      //ImGui::TableHeadersRow();

      //max MDL
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted("Max Mastering Display Luminance");
      ImGui::SetItemTooltip("max MDL");
      ImGui::TableSetColumnIndex(1);
      modified |= ImGui::InputInt("##maxMDL CLL", &Global.hdr_metadata.max_mdl, 0);

      //min MDL
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted("Min Mastering Display Luminance");
      ImGui::SetItemTooltip("min MDL");
      ImGui::TableSetColumnIndex(1);
      modified |= ImGui::InputFloat("##minMDL CLL", &Global.hdr_metadata.min_mdl, 0.0, 0.0, "%.4lf");

      //max CLL
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted("Max Content Light Level");
      ImGui::SetItemTooltip("max CLL");
      ImGui::TableSetColumnIndex(1);
      modified |= ImGui::InputInt("##maxCLL CLL", &Global.hdr_metadata.max_cll, 0);

      //max FALL
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted("Max Frame Average Light Level");
      ImGui::SetItemTooltip("max FALL");
      ImGui::TableSetColumnIndex(1);
      modified |= ImGui::InputInt("##maxFALL CLL", &Global.hdr_metadata.max_fall, 0);

      ImGui::EndTable();
    }

    if (ImGui::BeginTable("table for primaries", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
      //header
      ImGui::TableSetupColumn("primary");
      ImGui::TableSetupColumn("x coordinate");
      ImGui::TableSetupColumn("y coordinate");
      ImGui::TableHeadersRow();

      //primary red
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "red");
      ImGui::TableSetColumnIndex(1);
      modified |= ImGui::InputDouble("##red x", &Global.hdr_metadata.primary_red[0], 0.0, 0.0, "%.4lf");
      ImGui::TableSetColumnIndex(2);
      modified |= ImGui::InputDouble("##red y", &Global.hdr_metadata.primary_red[1], 0.0, 0.0, "%.4lf");

      //primary green
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextColored(ImVec4(0.3f, 1.f, 0.3f, 1.f), "green");
      ImGui::TableSetColumnIndex(1);
      modified |= ImGui::InputDouble("##green x", &Global.hdr_metadata.primary_green[0], 0.0, 0.0, "%.4lf");
      ImGui::TableSetColumnIndex(2);
      modified |= ImGui::InputDouble("##green y", &Global.hdr_metadata.primary_green[1], 0.0, 0.0, "%.4lf");

      //primary blue
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextColored(ImVec4(0.3f, 0.3f, 1.f, 1.f), "blue");
      ImGui::TableSetColumnIndex(1);
      modified |= ImGui::InputDouble("##blue x", &Global.hdr_metadata.primary_blue[0], 0.0, 0.0, "%.4lf");
      ImGui::TableSetColumnIndex(2);
      modified |= ImGui::InputDouble("##blue y", &Global.hdr_metadata.primary_blue[1], 0.0, 0.0, "%.4lf");

      ImGui::EndTable();
    }

    if (ImGui::BeginTable("table for white point", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
      //header
      ImGui::TableSetupColumn("");
      ImGui::TableSetupColumn("x coordinate");
      ImGui::TableSetupColumn("y coordinate");
      ImGui::TableHeadersRow();

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "white point");
      ImGui::TableSetColumnIndex(1);
      modified |= ImGui::InputDouble("##white point x", &Global.hdr_metadata.white_point[0], 0.0, 0.0, "%.5lf");
      ImGui::TableSetColumnIndex(2);
      modified |= ImGui::InputDouble("##white point y", &Global.hdr_metadata.white_point[1], 0.0, 0.0, "%.5lf");

      ImGui::EndTable();
    }


    if (ImGui::Button("Set HDR Metadata"))
    {
      ImGui::OpenPopup("SetHDRMetaData_message_popup");

      const std::lock_guard<std::mutex>lock(Global.mutex);

      IDXGISwapChain* swapchain = reinterpret_cast<IDXGISwapChain*>(Global.swapchain->get_native());
      IDXGISwapChain4* swapchain4;

      if (SUCCEEDED(swapchain->QueryInterface(__uuidof(IDXGISwapChain4), reinterpret_cast<void**>(&swapchain4))))
      {
        DXGI_HDR_METADATA_HDR10 hdr10_metadata = Get_DXGI_HDR10_Metadata();

        if (SUCCEEDED(swapchain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &hdr10_metadata)))
        {
          Global.sethdrmetadata_message = "HDR metadata was successfully set.";
        }
        else
        {
          Global.sethdrmetadata_message = "Setting the HDR metadata failed!";
          Global.sethdrmetadata_message_colour = ImVec4(1.f, 0.3f, 0.3f, 1.f);
        }

        swapchain4->Release();
      }
      else
      {
        Global.sethdrmetadata_message = "Could not query the swapchain interface!";
        Global.sethdrmetadata_message_colour = ImVec4(1.f, 0.3f, 0.3f, 1.f);
      }

    }

    if (ImGui::Button("Unset HDR Metadata"))
    {
      ImGui::OpenPopup("SetHDRMetaData_message_popup");

      const std::lock_guard<std::mutex>lock(Global.mutex);

      IDXGISwapChain* swapchain = reinterpret_cast<IDXGISwapChain*>(Global.swapchain->get_native());
      IDXGISwapChain4* swapchain4;

      if (SUCCEEDED(swapchain->QueryInterface(__uuidof(IDXGISwapChain4), reinterpret_cast<void**>(&swapchain4))))
      {
        if (SUCCEEDED(swapchain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr)))
        {
          Global.sethdrmetadata_message = "HDR metadata was successfully unset.";
        }
        else
        {
          Global.sethdrmetadata_message = "Unsetting the HDR metadata failed!";
          Global.sethdrmetadata_message_colour = ImVec4(1.f, 0.3f, 0.3f, 1.f);
        }

        swapchain4->Release();
      }
      else
      {
        Global.sethdrmetadata_message = "Could not query the swapchain interface!";
        Global.sethdrmetadata_message_colour = ImVec4(1.f, 0.3f, 0.3f, 1.f);
      }
    }

    if (ImGui::BeginPopup("SetHDRMetaData_message_popup"))
    {
      ImGui::TextColored(Global.sethdrmetadata_message_colour, "%s", Global.sethdrmetadata_message.c_str());
      ImGui::EndPopup();
    }

    if (ImGui::Button("Reset input fields to default"))
    {
      Global.hdr_metadata.primary_red[0]   = DEFAULT_PRIMARY_RED[0];
      Global.hdr_metadata.primary_red[1]   = DEFAULT_PRIMARY_RED[1];
      Global.hdr_metadata.primary_green[0] = DEFAULT_PRIMARY_GREEN[0];
      Global.hdr_metadata.primary_green[1] = DEFAULT_PRIMARY_GREEN[1];
      Global.hdr_metadata.primary_blue[0]  = DEFAULT_PRIMARY_BLUE[0];
      Global.hdr_metadata.primary_blue[1]  = DEFAULT_PRIMARY_BLUE[1];
      Global.hdr_metadata.white_point[0]   = DEFAULT_WHITE_POINT[0];
      Global.hdr_metadata.white_point[1]   = DEFAULT_WHITE_POINT[1];

      Global.hdr_metadata.max_mdl  = DEFAULT_MAX_MDL;
      Global.hdr_metadata.min_mdl  = DEFAULT_MIN_MDL;
      Global.hdr_metadata.max_cll  = DEFAULT_MAX_CLL;
      Global.hdr_metadata.max_fall = DEFAULT_MAX_FALL;

      modified = true;
    }

    if (modified)
    {
      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "prim_red_x", Global.hdr_metadata.primary_red[0]);
      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "prim_red_y", Global.hdr_metadata.primary_red[1]);

      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "prim_green_x", Global.hdr_metadata.primary_green[0]);
      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "prim_green_y", Global.hdr_metadata.primary_green[1]);

      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "prim_blue_x", Global.hdr_metadata.primary_blue[0]);
      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "prim_blue_y", Global.hdr_metadata.primary_blue[1]);

      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "white_point_x", Global.hdr_metadata.white_point[0]);
      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "white_point_y", Global.hdr_metadata.white_point[1]);

      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "max_mdl", Global.hdr_metadata.max_mdl);
      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "min_mdl", Global.hdr_metadata.min_mdl);

      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "max_cll",  Global.hdr_metadata.max_cll);
      reshade::set_config_value(nullptr, "ReShade_HDR_Metadata", "max_fall", Global.hdr_metadata.max_fall);
    }
  }
  else
  {
    ImGui::TextUnformatted("Unsupported API!");
  }
}


static void On_Init_Device(reshade::api::device* Device)
{
  const std::lock_guard<std::mutex>lock(Global.mutex);

  Global.device = Device;

  reshade::api::device_api device_api = Device->get_api();
  // only need to set it true because:
  // - the default is false
  // - on destroy device it will be always set to false
  if (device_api == reshade::api::device_api::d3d10
   || device_api == reshade::api::device_api::d3d11
   || device_api == reshade::api::device_api::d3d12)
  {
    Global.api_is_supported = true;
  }

  return;
}

static void On_Destroy_Device(reshade::api::device* Device)
{
  const std::lock_guard<std::mutex>lock(Global.mutex);

  Global.device = nullptr;

  Global.api_is_supported = false;

  return;
}

static void On_Init_Swapchain(reshade::api::swapchain* Swapchain, bool Resize)
{
  const std::lock_guard<std::mutex>lock(Global.mutex);

  if (Global.api_is_supported)
  {
    Global.swapchain = Swapchain;
  }

  return;
}

static void On_Destroy_Swapchain(reshade::api::swapchain* Swapchain, bool Resize)
{
  const std::lock_guard<std::mutex>lock(Global.mutex);

  Global.swapchain = nullptr;

  return;
}

static void On_Init_Effect_Runtime(reshade::api::effect_runtime* runtime)
{
  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "prim_red_x", Global.hdr_metadata.primary_red[0]);
  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "prim_red_y", Global.hdr_metadata.primary_red[1]);

  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "prim_green_x", Global.hdr_metadata.primary_green[0]);
  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "prim_green_y", Global.hdr_metadata.primary_green[1]);

  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "prim_blue_x", Global.hdr_metadata.primary_blue[0]);
  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "prim_blue_y", Global.hdr_metadata.primary_blue[1]);

  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "white_point_x", Global.hdr_metadata.white_point[0]);
  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "white_point_y", Global.hdr_metadata.white_point[1]);

  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "max_mdl", Global.hdr_metadata.max_mdl);
  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "min_mdl", Global.hdr_metadata.min_mdl);

  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "max_cll",  Global.hdr_metadata.max_cll);
  reshade::get_config_value(nullptr, "ReShade_HDR_Metadata", "max_fall", Global.hdr_metadata.max_fall);
}


extern "C" __declspec(dllexport) constexpr const char* NAME = "ReShade HDR Metadata";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Set HDR Metadata to your liking.";

BOOL APIENTRY DllMain(HMODULE H_Module, DWORD FDW_Reason, LPVOID)
{
  switch (FDW_Reason)
  {
    case DLL_PROCESS_ATTACH:
    {
      if (!reshade::register_addon(H_Module))
      {
        return FALSE;
      }

      reshade::register_event<reshade::addon_event::init_device>(On_Init_Device);
      reshade::register_event<reshade::addon_event::destroy_device>(On_Destroy_Device);

      reshade::register_event<reshade::addon_event::init_swapchain>(On_Init_Swapchain);
      reshade::register_event<reshade::addon_event::destroy_swapchain>(On_Destroy_Swapchain);

      reshade::register_event<reshade::addon_event::init_effect_runtime>(On_Init_Effect_Runtime);

      reshade::register_overlay("HDR Metadata", Draw_UI);
    }
    break;

    case DLL_PROCESS_DETACH:
    {
      reshade::unregister_event<reshade::addon_event::init_device>(On_Init_Device);
      reshade::unregister_event<reshade::addon_event::destroy_device>(On_Destroy_Device);

      reshade::unregister_event<reshade::addon_event::init_swapchain>(On_Init_Swapchain);
      reshade::unregister_event<reshade::addon_event::destroy_swapchain>(On_Destroy_Swapchain);

      reshade::unregister_event<reshade::addon_event::init_effect_runtime>(On_Init_Effect_Runtime);

      reshade::unregister_overlay("HDR Metadata", Draw_UI);
    }
    break;
  }

  return TRUE;
}
