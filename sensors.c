/**
 * lm-sensors plugin to lxpanel
 *
 * Copyright (C) 2012 by Dan Amlund Thomsen <dan@danamlund.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include <sensors/sensors.h>

#include <lxpanel/plugin.h>


/* from lxpanel-0.5.8/src/plugin.h */
gboolean plugin_button_press_event(GtkWidget *widget, 
                                   GdkEventButton *event, Plugin *plugin);

/* from lxpanel-0.5.8/src/configurator.h */
GtkWidget* create_generic_config_dlg( const char* title, GtkWidget* parent,
                                      GSourceFunc apply_func, Plugin * plugin,
                                      const char* name, ... );

/* from lxpanel-0.5.8/src/misc.h */
guint32 gcolor2rgb24(GdkColor *color);

typedef struct {
  const sensors_chip_name *chip;
  const sensors_feature *feature;
  int chip_nr, feature_nr, in_fahrenheit, hide_unit;
  char *col_normal_str, *col_warning_str, *col_critical_str;
  GdkColor col_normal, col_warning, col_critical;
  unsigned int timer;
  GtkWidget *label;
  GtkComboBox *combo_box;
} SensorsPlugin; 

static int sensors_plugins_running = 0;

static char* feature_type_name(int type) {
  switch (type) {
  case SENSORS_FEATURE_IN: return "in(?)";
  case SENSORS_FEATURE_FAN: return "fan";
  case SENSORS_FEATURE_TEMP: return "thermometer";
  case SENSORS_FEATURE_POWER: return "power";
  case SENSORS_FEATURE_ENERGY: return "energy";
  case SENSORS_FEATURE_CURR: return "current";
  case SENSORS_FEATURE_HUMIDITY: return "humidity";
  case SENSORS_FEATURE_MAX_MAIN: return "max_main(?)";
  case SENSORS_FEATURE_VID: return "voltage identifier";
  case SENSORS_FEATURE_INTRUSION: return "intrusion";
  case SENSORS_FEATURE_MAX_OTHER: return "max_other(?)";
  case SENSORS_FEATURE_BEEP_ENABLE: return "beep_enable(?)";
  /* case SENSORS_FEATURE_UNKNOWN: return "unknown"; */
  default: return "unknown";
  }
}

static char* subfeature_type_name(int type) {
  switch (type) {
  case SENSORS_SUBFEATURE_IN_INPUT: return "input";
  case SENSORS_SUBFEATURE_IN_MIN: return "min";
  case SENSORS_SUBFEATURE_IN_MAX: return "max";
  case SENSORS_SUBFEATURE_IN_LCRIT: return "lcrit";
  case SENSORS_SUBFEATURE_IN_CRIT: return "crit";
  case SENSORS_SUBFEATURE_IN_ALARM: return "alarm";
  case SENSORS_SUBFEATURE_IN_MIN_ALARM: return "min_alarm";
  case SENSORS_SUBFEATURE_IN_MAX_ALARM: return "max_alarm";
  case SENSORS_SUBFEATURE_IN_BEEP: return "beep";
  case SENSORS_SUBFEATURE_IN_LCRIT_ALARM: return "lcrit_alarm";
  case SENSORS_SUBFEATURE_IN_CRIT_ALARM: return "crit_alarm";
  case SENSORS_SUBFEATURE_FAN_INPUT: return "input";
  case SENSORS_SUBFEATURE_FAN_MIN: return "min";
  case SENSORS_SUBFEATURE_FAN_ALARM: return "alarm";
  case SENSORS_SUBFEATURE_FAN_FAULT: return "fault";
  case SENSORS_SUBFEATURE_FAN_DIV: return "div";
  case SENSORS_SUBFEATURE_FAN_BEEP: return "beep";
  case SENSORS_SUBFEATURE_FAN_PULSES: return "pulses";
  case SENSORS_SUBFEATURE_TEMP_INPUT: return "input";
  case SENSORS_SUBFEATURE_TEMP_MAX: return "max";
  case SENSORS_SUBFEATURE_TEMP_MAX_HYST: return "max_hyst";
  case SENSORS_SUBFEATURE_TEMP_MIN: return "min";
  case SENSORS_SUBFEATURE_TEMP_CRIT: return "crit";
  case SENSORS_SUBFEATURE_TEMP_CRIT_HYST: return "crit_hyst";
  case SENSORS_SUBFEATURE_TEMP_LCRIT: return "lcrit";
  case SENSORS_SUBFEATURE_TEMP_EMERGENCY: return "emergency";
  case SENSORS_SUBFEATURE_TEMP_EMERGENCY_HYST: return "emergency_hyst";
  case SENSORS_SUBFEATURE_TEMP_ALARM: return "alarm";
  case SENSORS_SUBFEATURE_TEMP_MAX_ALARM: return "max_alarm";
  case SENSORS_SUBFEATURE_TEMP_MIN_ALARM: return "min_alarm";
  case SENSORS_SUBFEATURE_TEMP_CRIT_ALARM: return "crit_alarm";
  case SENSORS_SUBFEATURE_TEMP_FAULT: return "fault";
  case SENSORS_SUBFEATURE_TEMP_TYPE: return "type";
  case SENSORS_SUBFEATURE_TEMP_OFFSET: return "offset";
  case SENSORS_SUBFEATURE_TEMP_BEEP: return "beep";
  case SENSORS_SUBFEATURE_TEMP_EMERGENCY_ALARM: return "emergency_alarm";
  case SENSORS_SUBFEATURE_TEMP_LCRIT_ALARM: return "lcrit_alarm";
  case SENSORS_SUBFEATURE_POWER_AVERAGE: return "average";
  case SENSORS_SUBFEATURE_POWER_AVERAGE_HIGHEST: return "average_highest";
  case SENSORS_SUBFEATURE_POWER_AVERAGE_LOWEST: return "average_lowest";
  case SENSORS_SUBFEATURE_POWER_INPUT: return "input";
  case SENSORS_SUBFEATURE_POWER_INPUT_HIGHEST: return "input_highest";
  case SENSORS_SUBFEATURE_POWER_INPUT_LOWEST: return "input_lowest";
  case SENSORS_SUBFEATURE_POWER_CAP: return "cap";
  case SENSORS_SUBFEATURE_POWER_CAP_HYST: return "cap_hyst";
  case SENSORS_SUBFEATURE_POWER_MAX: return "max";
  case SENSORS_SUBFEATURE_POWER_CRIT: return "crit";
  case SENSORS_SUBFEATURE_POWER_AVERAGE_INTERVAL: return "average_interval";
  case SENSORS_SUBFEATURE_POWER_ALARM: return "alarm";
  case SENSORS_SUBFEATURE_POWER_CAP_ALARM: return "cap_alarm";
  case SENSORS_SUBFEATURE_POWER_MAX_ALARM: return "max_alarm";
  case SENSORS_SUBFEATURE_POWER_CRIT_ALARM: return "crit_alam";
  case SENSORS_SUBFEATURE_ENERGY_INPUT: return "input";
  case SENSORS_SUBFEATURE_CURR_INPUT: return "input";
  case SENSORS_SUBFEATURE_CURR_MIN: return "min";
  case SENSORS_SUBFEATURE_CURR_MAX: return "max";
  case SENSORS_SUBFEATURE_CURR_LCRIT: return "lcrit";
  case SENSORS_SUBFEATURE_CURR_CRIT: return "crit";
  case SENSORS_SUBFEATURE_CURR_ALARM: return "alarm";
  case SENSORS_SUBFEATURE_CURR_MIN_ALARM: return "min_alarm";
  case SENSORS_SUBFEATURE_CURR_MAX_ALARM: return "max_alarm";
  case SENSORS_SUBFEATURE_CURR_BEEP: return "beep";
  case SENSORS_SUBFEATURE_CURR_LCRIT_ALARM: return "lcrit_alarm";
  case SENSORS_SUBFEATURE_CURR_CRIT_ALARM: return "crit_alarm";
  case SENSORS_SUBFEATURE_HUMIDITY_INPUT: return "input";
  case SENSORS_SUBFEATURE_VID: return "vid";
  case SENSORS_SUBFEATURE_INTRUSION_ALARM: return "alarm";
  case SENSORS_SUBFEATURE_INTRUSION_BEEP: return "beep";
  case SENSORS_SUBFEATURE_BEEP_ENABLE: return "beep_enable";
  /* case SENSORS_SUBFEATURE_UNKNOWN: return "unknown"; */
  default: return "unknown";
  }
}

static inline double deg_ctof(double cel) {
  return cel * (9.0F / 5.0F) + 32.0F;
}

static char* sensor_reading(const sensors_chip_name *chip,
                            const sensors_feature *feature,
                            SensorsPlugin *sp) {
  char out[256], out0[256];
  out[0] = out0[0] = '\0';
  GdkColor color;

  char error[] = "read-error";

  if (chip == NULL || feature == NULL) {
    strcpy(out, error);
    goto end_of_func;
  }

  const sensors_subfeature *sf;
  double temp, max, crit, rpm, volt, min;
  switch (feature->type) {
  case SENSORS_FEATURE_IN:
    volt = min = max = NAN;
    sf = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_IN_INPUT);
    if (sf != NULL) sensors_get_value(chip, sf->number, &volt);
    sf = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_IN_MIN);
    if (sf != NULL) sensors_get_value(chip, sf->number, &min);
    sf = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_IN_MAX);
    if (sf != NULL) sensors_get_value(chip, sf->number, &max);

    if (isnan(temp)) {
      strcpy(out, error);
      goto end_of_func;
    }

    if (sp != NULL) {
      color = (((!isnan(max) && volt >= max) || (!isnan(min) && volt <= min)) 
               ? sp->col_normal : sp->col_critical);
      sprintf(out, "%.2lf%s", volt, (sp->hide_unit ? "" : " V"));
    } else {
      sprintf(out, "%.2lf V", volt);
      if ((!isnan(max) && volt >= max) ||
          (!isnan(min) && volt <= min)) {
        strcpy(out0, out);
        sprintf(out, "! %s !", out0);
      }
    }
    break;

  case SENSORS_FEATURE_TEMP:
    temp = max = crit = NAN;
    sf = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_INPUT);
    if (sf != NULL) sensors_get_value(chip, sf->number, &temp);
    sf = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_MAX);
    if (sf != NULL) sensors_get_value(chip, sf->number, &max);
    sf = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_CRIT);
    if (sf != NULL) sensors_get_value(chip, sf->number, &crit);

    if (isnan(temp)){
      strcpy(out, error);
      goto end_of_func;
    }

    if (sp != NULL) {
      color = sp->col_normal;
      if (!isnan(crit) && temp >= crit)
        color = sp->col_critical;
      else if (!isnan(max) && temp >= max)
        color = sp->col_warning;

      sprintf(out, "%.1lf%s", 
              (sp->in_fahrenheit ? deg_ctof(temp) : temp),
              (sp->hide_unit ? "" : (sp->in_fahrenheit ? "°F" : "°C")));
    } else {
      sprintf(out, "%.1lf°C", temp);
      if (!isnan(crit) && temp >= crit) {
        strcpy(out0, out);
        sprintf(out, "!!! %s !!!", out0);
      } else if (!isnan(max) && temp >= max) {
        strcpy(out0, out);
        sprintf(out, "! %s !", out);
      }
    }
    break;

  case SENSORS_FEATURE_FAN:
    rpm = NAN;
    sf = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_FAN_INPUT);
    if (sf != NULL) sensors_get_value(chip, sf->number, &rpm);
    
    if (isnan(rpm)) {
      strcpy(out, error);
      goto end_of_func;
    }
    if (sp != NULL) {
      color = sp->col_normal;
      sprintf(out, "%.0lf%s", rpm, (sp->hide_unit ? "" : " RPM"));
    } else
      sprintf(out, "%.0lf RPM", rpm);
    break;

  default:
    ;
    const sensors_subfeature *sf;
    int sfnr = 0;
    double val;
    while (NULL != (sf = sensors_get_all_subfeatures(chip, feature, &sfnr))) {
        val = NAN;
        sensors_get_value(chip, sf->number, &val);
        if (out[0] != '\0')
          sprintf(out, "%s, ", out);
        sprintf(out, "%s%s=%.2lf", out, subfeature_type_name(sf->type), val);
      }
    break;
  }

 end_of_func:
  
  if (sp != NULL) {
    strcpy(out0, out);
    sprintf(out, "<span color=\"#%06x\"><b>%s</b></span>", 
            gcolor2rgb24(&color), out0);
  }

  return strndup(out, 256);
}

static char* get_sensor_string(const sensors_chip_name *chip,
                               const sensors_feature *feature) {
  char out[256];
  char *reading = sensor_reading(chip, feature, NULL);
  char *label = sensors_get_label(chip, feature);
  sprintf(out, "%s: %s/%s (%s)", 
          feature_type_name(feature->type), chip->prefix, label, reading);
  free(reading);
  free(label);
  return strndup(out, 256);
}

static int update_display(SensorsPlugin *sp) {
  char *str = sensor_reading(sp->chip, sp->feature, sp);
  gtk_label_set_markup(GTK_LABEL(sp->label), str);
  free(str);
  return 1;
}

static int sensors_constructor(Plugin * p, char ** fp) {
  SensorsPlugin *sp = g_new0(SensorsPlugin, 1);

  line s;
  s.len = 256;
  if (fp != NULL) {
    while (lxpanel_get_line(fp, &s) != LINE_BLOCK_END)  {
      if (s.type == LINE_NONE) {
        ERR( "sensors: illegal token %s\n", s.str);
        return 0;
      }
      if (s.type == LINE_VAR) {
        if (g_ascii_strcasecmp(s.t[0], "ChipNr") == 0)
          sp->chip_nr = atoi(s.t[1]);
        else if (g_ascii_strcasecmp(s.t[0], "FeatureNr") == 0)
          sp->feature_nr = atoi(s.t[1]);
        else if (g_ascii_strcasecmp(s.t[0], "Fahrenheit") == 0)
          sp->in_fahrenheit = atoi(s.t[1]);
        else if (g_ascii_strcasecmp(s.t[0], "HideUnit") == 0)
          sp->hide_unit = atoi(s.t[1]);
        else if (g_ascii_strcasecmp(s.t[0], "ColorNormal") == 0)
          sp->col_normal_str = g_strdup(s.t[1]);
        else if (g_ascii_strcasecmp(s.t[0], "ColorWarning") == 0)
          sp->col_warning_str = g_strdup(s.t[1]);
        else if (g_ascii_strcasecmp(s.t[0], "ColorCritical") == 0)
          sp->col_critical_str = g_strdup(s.t[1]);
      }
    }
  }

  if (sp->col_normal_str == NULL)
    sp->col_normal_str = g_strdup("white");
  if (sp->col_warning_str == NULL)
    sp->col_warning_str = g_strdup("yellow");
  if (sp->col_critical_str == NULL)
    sp->col_critical_str = g_strdup("red");

  gdk_color_parse(sp->col_normal_str, &sp->col_normal);
  gdk_color_parse(sp->col_warning_str, &sp->col_warning);
  gdk_color_parse(sp->col_critical_str, &sp->col_critical);

  if (sensors_plugins_running == 0)
    sensors_init(NULL);
  sensors_plugins_running++;

  int temp_nr = sp->chip_nr;
  sp->chip = sensors_get_detected_chips(NULL, &temp_nr);
  if (sp->chip != NULL) {
    temp_nr = sp->feature_nr;
    sp->feature = sensors_get_features(sp->chip, &temp_nr);
  }

  p->priv = sp;

  GtkWidget *label = gtk_label_new("..");
  sp->label = label;
  p->pwid = gtk_event_box_new();
  gtk_container_set_border_width(GTK_CONTAINER(p->pwid), 3);
  gtk_container_add(GTK_CONTAINER(p->pwid), GTK_WIDGET(label));
  gtk_widget_set_has_window(p->pwid, FALSE);
  g_signal_connect (G_OBJECT (p->pwid), "button_press_event",
                    G_CALLBACK (plugin_button_press_event), (gpointer) p);
  gtk_widget_show_all(p->pwid);
  update_display(sp);
  sp->timer = g_timeout_add(1000, (GSourceFunc) update_display, (gpointer) sp);
  return 1;
}

static void sensors_destructor(Plugin * p) {
  SensorsPlugin *sp = p->priv;
  g_source_remove(sp->timer);
  g_free(sp->col_normal_str);
  g_free(sp->col_warning_str);
  g_free(sp->col_critical_str);
  g_free(sp);

  sensors_plugins_running--;
  if (sensors_plugins_running == 0)
    sensors_cleanup();
}

static void sensors_sensor_changed(GtkComboBox * cb, gpointer * data) {
  Plugin *p = (Plugin *) data;
  SensorsPlugin *sp = (SensorsPlugin *) p->priv;

  const sensors_chip_name *chip;
  const sensors_feature *feature;
  int snr = 0, cnr = 0, fnr;

  while (NULL != (chip = sensors_get_detected_chips(NULL, &cnr))) {
    fnr = 0;
    while (NULL != (feature = sensors_get_features(chip, &fnr))) {
      if (snr == gtk_combo_box_get_active(sp->combo_box)) {
        sp->chip_nr = cnr - 1;
        sp->chip = chip;
        sp->feature_nr = fnr - 1;
        sp->feature = feature;
        return;
      }
      snr++;
    }
  }
}

static void sensors_apply_configure(Plugin* p) {
  SensorsPlugin *sp = (SensorsPlugin *) p->priv;

  gdk_color_parse(sp->col_normal_str, &sp->col_normal);
  gdk_color_parse(sp->col_warning_str, &sp->col_warning);
  gdk_color_parse(sp->col_critical_str, &sp->col_critical);
}

static void sensors_configure(Plugin * p, GtkWindow * parent) {
  GtkWidget *dialog;
  SensorsPlugin *sp = (SensorsPlugin *) p->priv;
  dialog = create_generic_config_dlg
    (_(p->class->name),
     GTK_WIDGET(parent),
     (GSourceFunc) sensors_apply_configure, (gpointer) p,
     _("In Fahrenheit"), &sp->in_fahrenheit, CONF_TYPE_BOOL,
     _("Hide unit"), &sp->hide_unit, CONF_TYPE_BOOL,
     _("Normal text color"), &sp->col_normal_str, CONF_TYPE_STR,
     _("Warning text color"), &sp->col_warning_str, CONF_TYPE_STR,
     _("Critical text color"), &sp->col_critical_str, CONF_TYPE_STR,
     NULL);

  GtkWidget * hbox = gtk_hbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

  GtkWidget * label = gtk_label_new("Sensor:");
  gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 2 );

  GtkWidget * combo = gtk_combo_box_new_text();
  gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 2);
  sp->combo_box = GTK_COMBO_BOX(combo);

  const sensors_chip_name *chip;
  const sensors_feature *feature;
  char *str;
  int snr = 0, cnr = 0, fnr;

  while (NULL != (chip = sensors_get_detected_chips(NULL, &cnr))) {
    fnr = 0;
    while (NULL != (feature = sensors_get_features(chip, &fnr))) {
      str = get_sensor_string(chip, feature);
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), str);
      free(str);
      if (chip == sp->chip && feature == sp->feature) 
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), snr);
      snr++;
    }
  }
  g_signal_connect(combo, "changed", G_CALLBACK(sensors_sensor_changed), p);

  gtk_widget_show_all(dialog);
  gtk_window_present(GTK_WINDOW(dialog));
}

static void sensors_save_configuration(Plugin * p, FILE * fp) {
  SensorsPlugin * sp = (SensorsPlugin *) p->priv;
  lxpanel_put_int(fp, "ChipNr", sp->chip_nr);
  lxpanel_put_int(fp, "FeatureNr", sp->feature_nr);
  lxpanel_put_int(fp, "Fahrenheit", sp->in_fahrenheit);
  lxpanel_put_int(fp, "HideUnit", sp->hide_unit);
  lxpanel_put_str(fp, "ColorNormal", sp->col_normal_str);
  lxpanel_put_str(fp, "ColorWarning", sp->col_warning_str);
  lxpanel_put_str(fp, "ColorCritical", sp->col_critical_str);
}

PluginClass sensors_plugin_class = {
  PLUGINCLASS_VERSIONING,
  type : "sensors",
  name : N_("lm-sensors Monitor"),
  version: "1.0",
  description : N_("Monitors hardware sensors using lm-sensors."),

  one_per_system : FALSE,
  expand_available : FALSE,

  constructor : sensors_constructor,
  destructor  : sensors_destructor,
  config : sensors_configure,
  save : sensors_save_configuration
};
