import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/types/mount_config.dart';

class Mount with ChangeNotifier {
  final Auth _auth;
  final MountConfig mountConfig;
  final MountList? _mountList;
  bool _isMounting = false;
  bool _isRefreshing = false;

  Mount(this._auth, this.mountConfig, this._mountList, {isAdd = false}) {
    if (isAdd) {
      return;
    }
    refresh();
  }

  bool get autoStart => mountConfig.autoStart;
  String? get bucket => mountConfig.bucket;
  String get id => '${type}_$name';
  bool? get mounted => mountConfig.mounted;
  String get name => mountConfig.name;
  String get path => mountConfig.path;
  String get provider => mountConfig.provider;
  String get type => mountConfig.type;

  Future<void> _fetch() async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount?auth=$auth&name=$name&type=$type',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        return;
      }

      if (response.statusCode == 404) {
        _mountList?.reset();
        return;
      }

      if (response.statusCode != 200) {
        return;
      }

      if (_isMounting) {
        return;
      }

      mountConfig.updateSettings(jsonDecode(response.body));
      notifyListeners();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<void> _fetchStatus() async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount_status?auth=$auth&name=$name&type=$type',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        return;
      }

      if (response.statusCode == 404) {
        _mountList?.reset();
        return;
      }

      if (response.statusCode != 200) {
        return;
      }

      if (_isMounting) {
        return;
      }

      mountConfig.updateStatus(jsonDecode(response.body));
      notifyListeners();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<void> setMountAutoStart(bool autoStart) async {
    try {
      mountConfig.autoStart = autoStart;

      final auth = await _auth.createAuth();
      final response = await http.put(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount_auto_start?auth=$auth&name=$name&type=$type&auto_start=$autoStart',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        return;
      }

      if (response.statusCode == 404) {
        _mountList?.reset();
        return;
      }

      return refresh();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<void> setMountLocation(String location) async {
    try {
      mountConfig.path = location;

      final auth = await _auth.createAuth();
      final response = await http.put(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount_location?auth=$auth&name=$name&type=$type&location=$location',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        return;
      }

      if (response.statusCode == 404) {
        _mountList?.reset();
        return;
      }

      return refresh();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<List<String>> getAvailableLocations() async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull('${getBaseUri()}/api/v1/locations?auth=$auth'),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        return <String>[];
      }

      if (response.statusCode != 200) {
        return <String>[];
      }

      return (jsonDecode(response.body) as List).cast<String>();
    } catch (e) {
      debugPrint('$e');
    }

    return <String>[];
  }

  Future<String?> getMountLocation() async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount_location?auth=$auth&name=$name&type=$type',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        return null;
      }

      if (response.statusCode != 200) {
        return null;
      }

      final location = jsonDecode(response.body)['Location'] as String;
      return location.trim().isEmpty ? null : location;
    } catch (e) {
      debugPrint('$e');
    }

    return null;
  }

  Future<bool> mount(bool unmount, {String? location}) async {
    try {
      _isMounting = true;

      mountConfig.mounted = null;
      notifyListeners();

      var count = 0;
      while (_isRefreshing && count++ < 10) {
        await Future.delayed(Duration(seconds: 1));
      }

      final auth = await _auth.createAuth();
      final response = await http.post(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/mount?auth=$auth&unmount=$unmount&name=$name&type=$type&location=$location',
          ),
        ),
      );

      if (response.statusCode == 401) {
        displayAuthError(_auth);
        _auth.logoff();
        return false;
      }

      if (response.statusCode == 404) {
        _isMounting = false;
        _mountList?.reset();
        return true;
      }

      final badLocation = (!unmount && response.statusCode == 500);
      if (badLocation) {
        mountConfig.path = "";
      }

      await refresh(force: true);
      _isMounting = false;

      return !badLocation;
    } catch (e) {
      debugPrint('$e');
    }

    _isMounting = false;
    return true;
  }

  Future<void> refresh({bool force = false}) async {
    if (!force && (_isMounting || _isRefreshing)) {
      return;
    }

    _isRefreshing = true;

    try {
      await _fetch();
      await _fetchStatus();
    } catch (e) {
      debugPrint('$e');
    }

    _isRefreshing = false;
  }

  Future<void> remove() async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.delete(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/remove_mount?auth=$auth&name=$name&type=$type',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
      }

      if (response.statusCode == 200) {
        _mountList?.remove(name, type);
      }
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<void> setValue(String key, String value) async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.put(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/set_value_by_name?auth=$auth&name=$name&type=$type&key=$key&value=$value',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        return;
      }

      if (response.statusCode == 404) {
        _mountList?.reset();
        return;
      }

      if (response.statusCode != 200) {
        return;
      }

      return refresh();
    } catch (e) {
      debugPrint('$e');
    }
  }

  Future<bool> test() async {
    try {
      final map = await convertAllToString(
        jsonDecode(jsonEncode(mountConfig.settings)),
        _auth.key,
      );
      final auth = await _auth.createAuth();
      final response = await http.get(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/test?auth=$auth&name=$name&type=$type&config=${jsonEncode(map)}',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
      }

      return (response.statusCode == 200);
    } catch (e) {
      debugPrint('$e');
    }

    return false;
  }
}
