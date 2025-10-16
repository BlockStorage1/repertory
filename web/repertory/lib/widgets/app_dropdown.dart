import 'package:flutter/material.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';

class AppDropdown<T> extends StatelessWidget {
  const AppDropdown({
    super.key,
    required this.labelOf,
    required this.values,
    this.constrainToIntrinsic = false,
    this.contentPadding,
    this.dropdownColor,
    this.enabled = true,
    this.fillColor,
    this.isExpanded = false,
    this.labelText,
    this.maxWidth,
    this.onChanged,
    this.prefixIcon,
    this.textStyle,
    this.validator,
    this.value,
    this.widthMultiplier = 1.0,
  });

  final List<T> values;
  final String Function(T value) labelOf;
  final T? value;
  final ValueChanged<T?>? onChanged;
  final FormFieldValidator<T>? validator;
  final String? labelText;
  final IconData? prefixIcon;
  final bool enabled;
  final bool constrainToIntrinsic;
  final double widthMultiplier;
  final double? maxWidth;
  final bool isExpanded;
  final Color? dropdownColor;
  final TextStyle? textStyle;
  final EdgeInsetsGeometry? contentPadding;

  final Color? fillColor;

  double _measureTextWidth(
    BuildContext context,
    String text,
    TextStyle? style,
  ) {
    final tp = TextPainter(
      text: TextSpan(text: text, style: style),
      maxLines: 1,
      textDirection: Directionality.of(context),
    )..layout();
    return tp.width;
  }

  double? _computedMaxWidth(BuildContext context) {
    if (!constrainToIntrinsic) {
      return maxWidth;
    }

    final theme = Theme.of(context);
    final scheme = theme.colorScheme;
    final effectiveStyle =
        textStyle ??
        theme.textTheme.bodyMedium?.copyWith(color: scheme.onSurface);

    final longest = values.isEmpty
        ? ''
        : values
              .map((v) => labelOf(v))
              .reduce((a, b) => a.length >= b.length ? a : b);

    final labelW = _measureTextWidth(context, longest, effectiveStyle);

    final prefixW = prefixIcon == null ? 0.0 : 48.0;
    const arrowW = constants.largeIconSize;
    final pad = contentPadding ?? const EdgeInsets.all(constants.paddingSmall);
    final padW = (pad is EdgeInsets)
        ? (pad.left + pad.right)
        : constants.padding;

    final base = labelW + prefixW + arrowW + padW;

    final cap =
        maxWidth ?? (MediaQuery.of(context).size.width - constants.padding * 2);
    return (base * widthMultiplier).clamp(0.0, cap);
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final scheme = theme.colorScheme;

    final effectiveFill =
        fillColor ??
        darken(scheme.primary, 0.95).withValues(alpha: constants.dropDownAlpha);

    final effectiveTextStyle =
        textStyle ??
        theme.textTheme.bodyMedium?.copyWith(color: scheme.onSurface);

    final items = values.map((v) {
      return DropdownMenuItem<T>(
        value: v,
        child: Text(
          labelOf(v),
          style: effectiveTextStyle,
          overflow: TextOverflow.ellipsis,
        ),
      );
    }).toList();

    final field = DropdownButtonFormField<T>(
      decoration: createCommonDecoration(
        scheme,
        labelText ?? "",
        filled: true,
        icon: prefixIcon,
      ),
      dropdownColor: dropdownColor ?? effectiveFill,
      iconEnabledColor: scheme.onSurface,
      initialValue: value,
      isExpanded: isExpanded,
      items: items,
      onChanged: enabled ? onChanged : null,
      style: effectiveTextStyle,
      validator: validator,
    );

    final maxWidth = _computedMaxWidth(context);
    final wrapped = maxWidth == null
        ? field
        : ConstrainedBox(
            constraints: BoxConstraints(maxWidth: maxWidth),
            child: field,
          );

    return Align(
      alignment: Alignment.centerLeft,
      heightFactor: 1.0,
      child: wrapped,
    );
  }
}
