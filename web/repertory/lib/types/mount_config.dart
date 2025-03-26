import 'package:collection/collection.dart';
import 'package:repertory/helpers.dart' show initialCaps;

class MountConfig {
  bool? mounted;
  final String _name;
  String path = '';
  Map<String, dynamic> _settings = {};
  final String _type;
  MountConfig({required name, required type, Map<String, dynamic>? settings})
    : _name = name,
      _type = type {
    if (settings != null) {
      _settings = settings;
    }
  }

  String? get bucket => _settings['${provider}Config']?["Bucket"] as String;
  String get name => _name;
  String get provider => initialCaps(_type);
  UnmodifiableMapView<String, dynamic> get settings =>
      UnmodifiableMapView<String, dynamic>(_settings);
  String get type => _type;

  void updateSettings(Map<String, dynamic> settings) {
    _settings = settings;
  }

  void updateStatus(Map<String, dynamic> status) {
    path = status['Location'] as String;
    mounted = status['Active'] as bool;
  }
}
