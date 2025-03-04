import 'package:collection/collection.dart';
import 'package:flutter/material.dart';

class MountConfig {
  final String _name;
  String _path = "";
  Map<String, dynamic> _settings = {};
  IconData _state = Icons.toggle_off;
  final String _type;
  MountConfig({required name, required type}) : _name = name, _type = type;

  String get name => _name;
  String get path => _path;
  UnmodifiableMapView<String, dynamic> get settings =>
      UnmodifiableMapView<String, dynamic>(_settings);
  IconData get state => _state;
  String get type => _type;

  factory MountConfig.fromJson(String type, String name) {
    return MountConfig(name: name, type: type);
  }

  void updateSettings(Map<String, dynamic> settings) {
    _settings = settings;
  }

  void updateStatus(Map<String, dynamic> status) {
    _path = status["Location"] as String;
    _state = status["Active"] as bool ? Icons.toggle_on : Icons.toggle_off;
  }
}
