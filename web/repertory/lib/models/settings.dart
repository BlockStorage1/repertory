import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';

class Settings with ChangeNotifier {
  final Auth _auth;
  bool _autoStart = false;
  bool _enableAnimations = true;

  Settings(this._auth) {
    _auth.addListener(() {
      if (_auth.authenticated) {
        _fetch();
      }
    });
  }

  bool get autoStart => _autoStart;
  bool get enableAnimations => _enableAnimations;

  void _reset() {
    _autoStart = false;
  }

  Future<void> setEnableAnimations(bool value) async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.put(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/setting?auth=$auth&name=Animations&value=$value',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        _reset();
        return;
      }

      if (response.statusCode != 200) {
        _reset();
        return;
      }

      _enableAnimations = value;
      notifyListeners();
    } catch (e) {
      debugPrint('$e');
      _reset();
    }
  }

  Future<void> setAutoStart(bool value) async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.put(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/setting?auth=$auth&name=AutoStart&value=$value',
          ),
        ),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        _reset();
        return;
      }

      if (response.statusCode != 200) {
        _reset();
        return;
      }

      _autoStart = value;
      notifyListeners();
    } catch (e) {
      debugPrint('$e');
      _reset();
    }
  }

  Future<void> _fetch() async {
    try {
      final auth = await _auth.createAuth();
      final response = await http.get(
        Uri.parse(Uri.encodeFull('${getBaseUri()}/api/v1/settings?auth=$auth')),
      );

      if (response.statusCode == 401) {
        _auth.logoff();
        _reset();
        return;
      }

      if (response.statusCode != 200) {
        _reset();
        return;
      }

      final jsonData = jsonDecode(response.body);
      _enableAnimations = jsonData["Animations"] as bool;
      _autoStart = jsonData["AutoStart"] as bool;

      notifyListeners();
    } catch (e) {
      debugPrint('$e');
      _reset();
    }
  }
}
