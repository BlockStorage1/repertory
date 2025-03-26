import 'package:flutter/material.dart' show GlobalKey, NavigatorState;
import 'package:sodium_libs/sodium_libs.dart';

const addMountTitle = 'Add New Mount';
const appLogonTitle = 'Repertory Portal Login';
const appSettingsTitle = 'Portal Settings';
const appTitle = 'Repertory Management Portal';
const logonWidth = 300.0;
const databaseTypeList = ['rocksdb', 'sqlite'];
const downloadTypeList = ['default', 'direct', 'ring_buffer'];
const eventLevelList = ['critical', 'error', 'warn', 'info', 'debug', 'trace'];
const padding = 15.0;
const protocolTypeList = ['http', 'https'];
const providerTypeList = ['Encrypt', 'Remote', 'S3', 'Sia'];
const ringBufferSizeList = ['128', '256', '512', '1024', '2048'];

final GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();

Sodium? _sodium;
void setSodium(Sodium sodium) {
  _sodium = sodium;
}

Sodium get sodium => _sodium!;
