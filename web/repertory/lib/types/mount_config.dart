import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:repertory/helpers.dart' show initialCaps;

class MountConfig {
  final String _name;
  String _path = '';
  Map<String, dynamic> _settings = {};
  IconData? _state;
  final String _type;
  MountConfig({required name, required type, Map<String, dynamic>? settings})
    : _name = name,
      _type = type {
    if (settings != null) {
      _settings = settings;
    }
  }

  String? get bucket =>
      _settings['${initialCaps(_type)}Config']?["Bucket"] as String;
  String get name => _name;
  String get path => _path;
  UnmodifiableMapView<String, dynamic> get settings =>
      UnmodifiableMapView<String, dynamic>(_settings);
  IconData? get state => _state;
  String get type => _type;

  void updateSettings(Map<String, dynamic> settings) {
    _settings = settings;
  }

  void updateStatus(Map<String, dynamic> status) {
    _path = status['Location'] as String;
    _state = status['Active'] as bool ? Icons.toggle_on : Icons.toggle_off;
  }
}
