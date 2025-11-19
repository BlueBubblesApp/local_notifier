#include "include/local_notifier/local_notifier_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

#include "wintoastlib.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <codecvt>
#include <map>
#include <memory>
#include <sstream>

using namespace WinToastLib;

namespace {
std::unique_ptr<
    flutter::MethodChannel<flutter::EncodableValue>,
    std::default_delete<flutter::MethodChannel<flutter::EncodableValue>>>
    channel = nullptr;

class CustomToastHandler : public IWinToastHandler {
 public:
  CustomToastHandler(std::string identifier);

  void toastActivated() const {
    flutter::EncodableMap args = flutter::EncodableMap();
    args[flutter::EncodableValue("notificationId")] =
        flutter::EncodableValue(identifier);
    channel->InvokeMethod("onLocalNotificationClick",
                          std::make_unique<flutter::EncodableValue>(args));
  }

  void toastActivated(int actionIndex) const {
    flutter::EncodableMap args = flutter::EncodableMap();
    args[flutter::EncodableValue("notificationId")] =
        flutter::EncodableValue(identifier);
    args[flutter::EncodableValue("actionIndex")] =
        flutter::EncodableValue(actionIndex);
    channel->InvokeMethod("onLocalNotificationClickAction",
                          std::make_unique<flutter::EncodableValue>(args));
  }

  void toastActivated(std::wstring response) const {
    flutter::EncodableMap args = flutter::EncodableMap();
    args[flutter::EncodableValue("notificationId")] =
        flutter::EncodableValue(identifier);
    args[flutter::EncodableValue("input")] = flutter::EncodableValue(
        std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(response));
    channel->InvokeMethod("onLocalNotificationInput",
                          std::make_unique<flutter::EncodableValue>(args));
  }

  void toastDismissed(WinToastDismissalReason state) const {
    std::string closeReason = "unknown";

    switch (state) {
      case UserCanceled:
        closeReason = "userCanceled";
        break;
      case TimedOut:
        closeReason = "timedOut";
        break;
      default:
        break;
    }

    flutter::EncodableMap args = flutter::EncodableMap();
    args[flutter::EncodableValue("notificationId")] =
        flutter::EncodableValue(identifier);
    args[flutter::EncodableValue("closeReason")] =
        flutter::EncodableValue(closeReason);

    channel->InvokeMethod("onLocalNotificationClose",
                          std::make_unique<flutter::EncodableValue>(args));
  }

  void toastFailed() const {}

 private:
  std::string identifier;
};

CustomToastHandler::CustomToastHandler(std::string identifier) {
  this->identifier = identifier;
}

class LocalNotifierPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  LocalNotifierPlugin();

  virtual ~LocalNotifierPlugin();

 private:
  flutter::PluginRegistrarWindows* registrar;

  std::unordered_map<std::string, INT64> toast_id_map_ = {};

  HWND LocalNotifierPlugin::GetMainWindow();
  void LocalNotifierPlugin::Setup(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void LocalNotifierPlugin::Notify(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void LocalNotifierPlugin::Close(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

// static
void LocalNotifierPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      registrar->messenger(), "local_notifier",
      &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<LocalNotifierPlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto& call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

LocalNotifierPlugin::LocalNotifierPlugin() {}

LocalNotifierPlugin::~LocalNotifierPlugin() {}

HWND LocalNotifierPlugin::GetMainWindow() {
  return ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
}

void LocalNotifierPlugin::Setup(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (!WinToast::isCompatible()) {
    std::wcout << L"Error, your system in not supported!" << std::endl;
  }

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

  const flutter::EncodableMap& args =
      std::get<flutter::EncodableMap>(*method_call.arguments());

  std::string appName =
      std::get<std::string>(args.at(flutter::EncodableValue("appName")));
  std::string shortcutPolicy =
      std::get<std::string>(args.at(flutter::EncodableValue("shortcutPolicy")));

  WinToast::instance()->setAppName(converter.from_bytes(appName));
  WinToast::instance()->setAppUserModelId(converter.from_bytes(appName));
  if (shortcutPolicy.compare("ignore") == 0) {
    WinToast::instance()->setShortcutPolicy(WinToast::SHORTCUT_POLICY_IGNORE);
  } else if (shortcutPolicy.compare("requireNoCreate") == 0) {
    WinToast::instance()->setShortcutPolicy(
        WinToast::SHORTCUT_POLICY_REQUIRE_NO_CREATE);
  } else if (shortcutPolicy.compare("requireCreate") == 0) {
    WinToast::instance()->setShortcutPolicy(
        WinToast::SHORTCUT_POLICY_REQUIRE_CREATE);
  }
  WinToast::instance()->initialize();

  result->Success(flutter::EncodableValue(true));
}

void LocalNotifierPlugin::Notify(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (!WinToast::isCompatible()) {
    std::wcout << L"Error, your system in not supported!" << std::endl;
  }

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

  const flutter::EncodableMap& args =
      std::get<flutter::EncodableMap>(*method_call.arguments());

  std::string identifier =
      std::get<std::string>(args.at(flutter::EncodableValue("identifier")));
  std::string type =
      std::get<std::string>(args.at(flutter::EncodableValue("type")));
  std::string title =
      std::get<std::string>(args.at(flutter::EncodableValue("title")));
  std::string body =
      std::get<std::string>(args.at(flutter::EncodableValue("body")));
  std::string body2 =
      std::get<std::string>(args.at(flutter::EncodableValue("body2")));
  std::string attributionText =
      std::get<std::string>(args.at(flutter::EncodableValue("attributionText")));
  std::string imagePath =
      std::get<std::string>(args.at(flutter::EncodableValue("imagePath")));
  std::string systemSound =
      std::get<std::string>(args.at(flutter::EncodableValue("systemSound")));
  std::string soundOption =
      std::get<std::string>(args.at(flutter::EncodableValue("soundOption")));
  bool hasInput = std::get<bool>(args.at(flutter::EncodableValue("hasInput")));
  std::string inputPlaceholder = std::get<std::string>(
      args.at(flutter::EncodableValue("inputPlaceholder")));
  std::string inputButtonText = std::get<std::string>(
      args.at(flutter::EncodableValue("inputButtonText")));

  flutter::EncodableList actions = std::get<flutter::EncodableList>(
      args.at(flutter::EncodableValue("actions")));

  WinToastTemplate::WinToastTemplateType templateType;
  if (type == "LocalNotificationType.text01") {
    templateType = WinToastTemplate::WinToastTemplateType::Text01;
  } else if (type == "LocalNotificationType.text02") {
    templateType = WinToastTemplate::WinToastTemplateType::Text02;
  } else if (type == "LocalNotificationType.text03") {
    templateType = WinToastTemplate::WinToastTemplateType::Text03;
  } else if (type == "LocalNotificationType.text04") {
    templateType = WinToastTemplate::WinToastTemplateType::Text04;
  } else if (type == "LocalNotificationType.imageAndText01") {
    templateType = WinToastTemplate::WinToastTemplateType::ImageAndText01;
  } else if (type == "LocalNotificationType.imageAndText02") {
    templateType = WinToastTemplate::WinToastTemplateType::ImageAndText02;
  } else if (type == "LocalNotificationType.imageAndText03") {
    templateType = WinToastTemplate::WinToastTemplateType::ImageAndText03;
  } else {
    templateType = WinToastTemplate::WinToastTemplateType::ImageAndText04;
  }

  WinToastTemplate toast = WinToastTemplate(templateType);

  toast.setFirstLine(converter.from_bytes(title));
  if (body.size() != 0) {
    toast.setSecondLine(converter.from_bytes(body));
  }
  if (body2.size() != 0) {
    toast.setThirdLine(converter.from_bytes(body2));
  }
  if (attributionText.size() != 0) {
    toast.setAttributionText(converter.from_bytes(attributionText));
  }
  if (imagePath.size() != 0) {
    toast.setImagePath(converter.from_bytes(imagePath));
  }
  if (systemSound == "LocalNotificationSound.defaultSound") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::DefaultSound);
  } else if (systemSound == "LocalNotificationSound.im") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::IM);
  } else if (systemSound == "LocalNotificationSound.mail") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Mail);
  } else if (systemSound == "LocalNotificationSound.reminder") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Reminder);
  } else if (systemSound == "LocalNotificationSound.sms") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::SMS);
  } else if (systemSound == "LocalNotificationSound.alarm") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm);
  } else if (systemSound == "LocalNotificationSound.alarm2") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm2);
  } else if (systemSound == "LocalNotificationSound.alarm3") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm3);
  } else if (systemSound == "LocalNotificationSound.alarm4") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm4);
  } else if (systemSound == "LocalNotificationSound.alarm5") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm5);
  } else if (systemSound == "LocalNotificationSound.alarm6") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm6);
  } else if (systemSound == "LocalNotificationSound.alarm7") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm7);
  } else if (systemSound == "LocalNotificationSound.alarm8") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm8);
  } else if (systemSound == "LocalNotificationSound.alarm9") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm9);
  } else if (systemSound == "LocalNotificationSound.alarm10") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Alarm10);
  } else if (systemSound == "LocalNotificationSound.call") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call);
  } else if (systemSound == "LocalNotificationSound.call1") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call1);
  } else if (systemSound == "LocalNotificationSound.call2") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call2);
  } else if (systemSound == "LocalNotificationSound.call3") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call3);
  } else if (systemSound == "LocalNotificationSound.call4") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call4);
  } else if (systemSound == "LocalNotificationSound.call5") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call5);
  } else if (systemSound == "LocalNotificationSound.call6") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call6);
  } else if (systemSound == "LocalNotificationSound.call7") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call7);
  } else if (systemSound == "LocalNotificationSound.call8") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call8);
  } else if (systemSound == "LocalNotificationSound.call9") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call9);
  } else if (systemSound == "LocalNotificationSound.call10") {
    toast.setAudioPath(WinToastTemplate::AudioSystemFile::Call10);
  }

  if (soundOption == "LocalNotificationSoundOption.defaultOption") {
    toast.setAudioOption(WinToastTemplate::AudioOption::Default);
  } else if (soundOption == "LocalNotificationSoundOption.silent") {
    toast.setAudioOption(WinToastTemplate::AudioOption::Silent);
  } else if (soundOption == "LocalNotificationSoundOption.loop") {
    toast.setAudioOption(WinToastTemplate::AudioOption::Loop);
  }

  for (flutter::EncodableValue action_value : actions) {
    flutter::EncodableMap action_map =
        std::get<flutter::EncodableMap>(action_value);
    std::string action_text =
        std::get<std::string>(action_map.at(flutter::EncodableValue("text")));

    toast.addAction(converter.from_bytes(action_text));
  }

  if (hasInput) {
    toast.addInput();

    if (inputPlaceholder.size() != 0) {
      toast.setInputPlaceholder(converter.from_bytes(inputPlaceholder));
    }

    toast.setInputButtonText(converter.from_bytes(inputButtonText));
  }

  std::string duration =
      std::get<std::string>(args.at(flutter::EncodableValue("duration")));

  if (duration == "LocalNotificationDuration.short") {
    toast.setDuration(WinToastTemplate::Duration::Short);
  } else if (duration == "LocalNotificationDuration.long") {
    toast.setDuration(WinToastTemplate::Duration::Long);
  } else {
    toast.setDuration(WinToastTemplate::Duration::System);
  }

  CustomToastHandler* handler = new CustomToastHandler(identifier);
  INT64 toast_id = WinToast::instance()->showToast(toast, handler);

  toast_id_map_.insert(std::make_pair(identifier, toast_id));

  flutter::EncodableMap args2 = flutter::EncodableMap();
  args2[flutter::EncodableValue("notificationId")] =
      flutter::EncodableValue(identifier);
  channel->InvokeMethod("onLocalNotificationShow",
                        std::make_unique<flutter::EncodableValue>(args2));

  result->Success(flutter::EncodableValue(true));
}

void LocalNotifierPlugin::Close(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const flutter::EncodableMap& args =
      std::get<flutter::EncodableMap>(*method_call.arguments());

  std::string identifier =
      std::get<std::string>(args.at(flutter::EncodableValue("identifier")));

  if (toast_id_map_.find(identifier) != toast_id_map_.end()) {
    INT64 toast_id = toast_id_map_.at(identifier);

    WinToast::instance()->hideToast(toast_id);
    toast_id_map_.erase(identifier);
  }

  result->Success(flutter::EncodableValue(true));
}

void LocalNotifierPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("setup") == 0) {
    Setup(method_call, std::move(result));
  } else if (method_call.method_name().compare("notify") == 0) {
    Notify(method_call, std::move(result));
  } else if (method_call.method_name().compare("close") == 0) {
    Close(method_call, std::move(result));
  } else {
    result->NotImplemented();
  }
}

}  // namespace

void LocalNotifierPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  LocalNotifierPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
