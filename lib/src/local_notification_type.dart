enum LocalNotificationType {
  imageAndText01(lines: 1),
  imageAndText02(lines: 2),
  imageAndText03(lines: 2),
  imageAndText04(lines: 3),
  text01(lines: 1),
  text02(lines: 2),
  text03(lines: 2),
  text04(lines: 3);

  const LocalNotificationType({required this.lines});

  final int lines;

  get supportsImage => index < LocalNotificationType.text01.index;
}
