import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:sodium_libs/sodium_libs.dart';

class Auth with ChangeNotifier {
  bool _authenticated = false;
  SecureKey _key = SecureKey.random(constants.sodium, 32);
  String _user = "";
  MountList? mountList;

  bool get authenticated => _authenticated;
  SecureKey get key => _key;

  Future<void> authenticate(String user, String password) async {
    final sodium = constants.sodium;

    final keyHash = sodium.crypto.genericHash(
      outLen: sodium.crypto.aeadXChaCha20Poly1305IETF.keyBytes,
      message: Uint8List.fromList(password.toCharArray()),
    );

    _authenticated = true;
    _key = SecureKey.fromList(sodium, keyHash);
    _user = user;

    notifyListeners();
  }

  Future<String> createAuth() async {
    try {
      final response = await http.get(
        Uri.parse(Uri.encodeFull('${getBaseUri()}/api/v1/nonce')),
      );

      if (response.statusCode != 200) {
        logoff();
        return "";
      }

      final nonce = jsonDecode(response.body)["nonce"];
      return encryptValue('${_user}_$nonce', key);
    } catch (e) {
      debugPrint('$e');
    }

    return "";
  }

  void logoff() {
    _authenticated = false;
    _key = SecureKey.random(constants.sodium, 32);
    _user = "";

    notifyListeners();

    mountList?.clear();
  }
}
