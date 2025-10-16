import 'package:flutter/material.dart';
import 'package:repertory/helpers.dart' as helpers;

class AppTextField extends StatelessWidget {
  const AppTextField({
    super.key,
    this.autofocus = false,
    this.controller,
    this.enabled = true,
    this.hintText,
    this.icon,
    this.keyboardType,
    this.labelText,
    this.maxLines = 1,
    this.obscureText = false,
    this.onChanged,
    this.onFieldSubmitted,
    this.suffixIcon,
    this.textInputAction,
    this.validator,
  });

  final bool autofocus;
  final TextEditingController? controller;
  final bool enabled;
  final String? hintText;
  final IconData? icon;
  final TextInputType? keyboardType;
  final String? labelText;
  final int? maxLines;
  final bool obscureText;
  final ValueChanged<String>? onChanged;
  final ValueChanged<String>? onFieldSubmitted;
  final Widget? suffixIcon;
  final TextInputAction? textInputAction;
  final FormFieldValidator<String>? validator;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    final decoration = helpers
        .createCommonDecoration(
          scheme,
          labelText ?? '',
          filled: true,
          hintText: hintText,
          icon: icon,
        )
        .copyWith(suffixIcon: suffixIcon);

    return TextFormField(
      autofocus: autofocus,
      controller: controller,
      decoration: decoration,
      enabled: enabled,
      keyboardType: keyboardType,
      maxLines: maxLines,
      obscureText: obscureText,
      onChanged: onChanged,
      onFieldSubmitted: onFieldSubmitted,
      textInputAction: textInputAction,
      validator: validator,
    );
  }
}
