// constants.dart

import 'package:flutter/material.dart' show GlobalKey, NavigatorState, Color;
import 'package:sodium_libs/sodium_libs.dart';

const accentBlue = Color(0xFF1050A0);
const addMountTitle = 'Add New Mount';
const appLogonTitle = 'Repertory Portal Login';
const appSettingsTitle = 'Portal Settings';
const appTitle = 'Repertory Management Portal';
const borderRadius = 16.0;
const borderRadiusSmall = borderRadius / 2.0;
const borderRadiusTiny = borderRadiusSmall / 2.0;
const boxShadowAlpha = 0.20;
const databaseTypeList = ['rocksdb', 'sqlite'];
const dialogAlpha = 0.95;
const downloadTypeList = ['default', 'direct', 'ring_buffer'];
const dropDownAlpha = 99.0;
const eventLevelList = ['critical', 'error', 'warn', 'info', 'debug', 'trace'];
const gradientColors = [Color(0xFF0A0F1F), Color(0xFF1B1C1F)];
const gradientColors2 = [Color(0x07FFFFFF), Color(0x00000000)];
const highlightAlpha = 0.10;
const largeIconSize = 32.0;
const loginIconSize = 54.0;
const logonWidth = 420.0;
const outlineAlpha = 0.15;
const padding = 16.0;
const paddingLarge = padding * 2.0;
const paddingMedium = 12.0;
const paddingSmall = padding / 2.0;
const primaryAlpha = 0.12;
const primarySurfaceAlpha = 92.0;
const protocolTypeList = ['http', 'https'];
const providerTypeList = ['Encrypt', 'Remote', 'S3', 'Sia'];
const ringBufferSizeList = ['128', '256', '512', '1024', '2048'];
const secondaryAlpha = 0.45;
const secondarySurfaceAlpha = 70.0;
const smallIconSize = 18.0;
const surfaceContainerLowDark = Color(0xFF292A2D);
const surfaceDark = Color(0xFF202124);

final GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();

Sodium? _sodium;
void setSodium(Sodium sodium) {
  _sodium = sodium;
}

Sodium get sodium => _sodium!;
