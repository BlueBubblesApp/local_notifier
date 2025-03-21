import 'dart:io';

import 'package:flutter/material.dart';
import 'package:local_notifier/src/local_notification_action.dart';
import 'package:local_notifier/src/local_notification_close_reason.dart';
import 'package:local_notifier/src/local_notification_duration.dart';
import 'package:local_notifier/src/local_notification_listener.dart';
import 'package:local_notifier/src/local_notification_sound.dart';
import 'package:local_notifier/src/local_notification_sound_option.dart';
import 'package:local_notifier/src/local_notification_type.dart';
import 'package:local_notifier/src/local_notifier.dart';
import 'package:uuid/uuid.dart';

class LocalNotification with LocalNotificationListener {
  LocalNotification({
    String? identifier,
    this.type,
    required this.title,
    this.subtitle,
    this.body,
    this.body2,
    this.imagePath,
    this.systemSound,
    this.soundOption = LocalNotificationSoundOption.defaultOption,
    this.silent = false,
    this.duration = LocalNotificationDuration.system,
    this.actions,
    this.hasInput = false,
    this.inputPlaceholder,
    this.inputButtonText = 'Send',
  }) {
    if (Platform.isWindows) {
      assert(type != null);
      int numTexts =
          [body, body2].lastIndexWhere((e) => e?.isNotEmpty ?? false) + 1;
      assert(numTexts < type!.lines);
      assert(imagePath == null || type!.supportsImage);
      assert((actions?.length ?? 0) + (hasInput ? 1 : 0) <= 5);
    }

    if (identifier != null) {
      this.identifier = identifier;
    }
    localNotifier.addListener(this);
  }

  factory LocalNotification.fromJson(Map<String, dynamic> json) {
    List<LocalNotificationAction>? actions;

    if (json['actions'] != null) {
      Iterable<dynamic> l = json['actions'] as List;
      actions = l
          .map(
            (item) =>
                LocalNotificationAction.fromJson(item as Map<String, dynamic>),
          )
          .toList();
    }

    return LocalNotification(
      identifier: json['identifier'] as String?,
      type: LocalNotificationType.values.firstWhere(
        (e) => e.toString() == json['type'],
        orElse: () => LocalNotificationType.imageAndText04,
      ),
      title: json['title'] as String,
      subtitle: json['subtitle'] as String?,
      body: json['body'] as String?,
      body2: json['body2'] as String?,
      imagePath: json['imagePath'] as String?,
      systemSound: LocalNotificationSound.values.firstWhere(
        (e) => e.toString() == json['systemSound'],
        orElse: () => LocalNotificationSound.defaultSound,
      ),
      soundOption: LocalNotificationSoundOption.values.firstWhere(
        (e) => e.toString() == json['soundOption'],
        orElse: () => LocalNotificationSoundOption.defaultOption,
      ),
      silent: json['silent'] as bool,
      duration: LocalNotificationDuration.values.firstWhere(
        (e) => e.toString() == json['duration'],
        orElse: () => LocalNotificationDuration.system,
      ),
      actions: actions,
      hasInput: json['hasInput'] as bool,
      inputPlaceholder: json['inputPlaceholder'] as String?,
      inputButtonText: json['inputButtonText'] as String,
    );
  }

  String identifier = const Uuid().v4();

  /// Representing the type type of the notification (Windows only).
  LocalNotificationType? type;

  /// Representing the title of the notification.
  String title;

  /// Representing the subtitle of the notification.
  String? subtitle;

  /// Representing the body of the notification.
  String? body;

  /// Representing the third line of the notification (Windows only).
  String? body2;

  /// Representing the image path of the notification (Windows only).
  String? imagePath;

  /// Representing the system sound of the notification (Windows only).
  LocalNotificationSound? systemSound;

  /// Representing the audio option of the notification (Windows only).
  LocalNotificationSoundOption soundOption;

  /// Representing whether the notification is silent (Windows only).
  bool silent;

  /// Representing the duration of the notification (Windows only).
  LocalNotificationDuration duration;

  /// Representing the actions of the notification.
  List<LocalNotificationAction>? actions;

  /// Representing whether the notification has an input field (Windows only).
  bool hasInput;

  /// Representing the placeholder of the input field (Windows only).
  String? inputPlaceholder;

  /// Representing the button text of the input (Windows only).
  String inputButtonText;

  VoidCallback? onShow;
  ValueChanged<LocalNotificationCloseReason>? onClose;
  VoidCallback? onClick;
  ValueChanged<int>? onClickAction;
  ValueChanged<String>? onInput;

  Map<String, dynamic> toJson() {
    return {
      'identifier': identifier,
      'type': type?.toString(),
      'title': title,
      'subtitle': subtitle ?? '',
      'body': body ?? '',
      'body2': body2 ?? '',
      'imagePath': imagePath ?? '',
      'systemSound': systemSound?.toString() ?? '',
      'soundOption': soundOption.toString(),
      'silent': silent,
      'duration': duration.toString(),
      'actions': (actions ?? []).map((e) => e.toJson()).toList(),
      'hasInput': hasInput,
      'inputPlaceholder': inputPlaceholder ?? '',
      'inputButtonText': inputButtonText,
    }..removeWhere((key, value) => value == null);
  }

  /// Immediately shows the notification to the user
  Future<void> show() {
    return localNotifier.notify(this);
  }

  /// Closes the notification immediately.
  Future<void> close() {
    return localNotifier.close(this);
  }

  /// Destroys the notification immediately.
  Future<void> destroy() {
    return localNotifier.destroy(this);
  }

  @override
  void onLocalNotificationShow(LocalNotification notification) {
    if (identifier != notification.identifier || onShow == null) {
      return;
    }
    onShow!();
  }

  @override
  void onLocalNotificationClose(
    LocalNotification notification,
    LocalNotificationCloseReason closeReason,
  ) {
    if (identifier != notification.identifier || onClose == null) {
      return;
    }
    onClose!(closeReason);
  }

  @override
  void onLocalNotificationClick(LocalNotification notification) {
    if (identifier != notification.identifier || onClick == null) {
      return;
    }
    onClick!();
  }

  @override
  void onLocalNotificationClickAction(
    LocalNotification notification,
    int actionIndex,
  ) {
    if (identifier != notification.identifier || onClickAction == null) {
      return;
    }
    onClickAction!(actionIndex);
  }

  @override
  void onLocalNotificationInput(
    LocalNotification notification,
    String input,
  ) {
    if (identifier != notification.identifier || onInput == null) {
      return;
    }
    onInput!(input);
  }
}
