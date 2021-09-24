#pragma once

/// @file   AC_DroneShowManager.h
/// @brief  Drone show state management library

#include <AP_Common/AP_Common.h>
#include <AP_Common/Location.h>
#include <AP_Math/AP_Math.h>
#include <AP_Notify/RGBLed.h>
#include <AP_Param/AP_Param.h>

struct sb_trajectory_s;
struct sb_trajectory_player_s;

struct sb_light_program_s;
struct sb_light_player_s;

struct sb_rgb_color_s;

class DroneShowLEDFactory;
class DroneShowLED;

#if CONFIG_HAL_BOARD == HAL_BOARD_SITL
#  include <AP_HAL/utility/Socket.h>
#endif

// Drone show mode stages
enum DroneShowModeStage {
    DroneShow_Off,
    DroneShow_Init,
    DroneShow_WaitForStartTime,
    DroneShow_Takeoff,
    DroneShow_Performing,
    DroneShow_RTL,
    DroneShow_Loiter,
    DroneShow_Landing,
    DroneShow_Landed,
    DroneShow_Error,
};

/// @class  AC_DroneShowManager
/// @brief  Class managing the trajectory and light program of a drone show
class AC_DroneShowManager {
public:
    AC_DroneShowManager();
    ~AC_DroneShowManager();

    /* Do not allow copies */
    AC_DroneShowManager(const AC_DroneShowManager &other) = delete;
    AC_DroneShowManager &operator=(const AC_DroneShowManager&) = delete;

    // Enum describing the possible sources of the start time currently set in the manager
    enum StartTimeSource {
        NONE = 0,         // No start time was set
        PARAMETER = 1,    // start time was set by the user via the START_TIME parameter
        START_METHOD = 2, // start time was set by calling the start_if_not_running() method
        RC_SWITCH = 3     // start time was set via the RC switch action
    };

    // Early initialization steps that have to be called early in the boot process
    // to ensure we have enough memory to do them even on low-memory boards like
    // a Pixhawk1
    void early_init();

    // Initializes the drone show subsystem at boot time
    void init();

    // Returns whether the user has asked the drone show manager to cancel the
    // show as soon as possible. This flag is checked regularly from
    // mode_drone_show.cpp
    bool cancel_requested() const { return _cancel_requested; }

    // Returns the color of the LED light on the drone according to its light
    // program the given number of seconds after the start time.
    void get_color_of_rgb_light_at_seconds(float time, struct sb_rgb_color_s* color);

    // Returns the desired position of the drone during the drone show the
    // given number of seconds after the start time, in the global coordinate
    // system, using centimeters as units.
    void get_desired_global_position_in_cms_at_seconds(float time, Location& loc);

    // Returns the desired velocity of the drone during the drone show the
    // given number of seconds after the start time, in the global NEU
    // cooordinate system, using centimeters as units.
    void get_desired_velocity_neu_in_cms_per_seconds_at_seconds(float time, Vector3f& vel);

    // Returns the landing time relative to the start of the show
    float get_relative_landing_time_sec() const { return _landing_time_sec; }

    // Returns the takeoff time relative to the start of the show
    float get_relative_takeoff_time_sec() const { return _takeoff_time_sec; }

    // Returns the start time in microseconds as a UNIX timestamp, as set by the user.
    uint64_t get_start_time_usec() const { return _start_time_usec; }

    // Returns the total duration of the loaded trajectory, in seconds
    float get_total_duration_sec() const { return _total_duration_sec; }

    // Returns the number of seconds elapsed since show start, in microseconds
    int64_t get_elapsed_time_since_start_usec() const;

    // Returns the number of seconds elapsed since show start, in seconds
    float get_elapsed_time_since_start_sec() const;

    // Returns a timestamp meant to be used solely for the purposes of implementing
    // light signals. The timestamp is synced to GPS seconds when the drone has
    // a good GPS fix.
    uint32_t _get_gps_synced_timestamp_in_millis_for_lights() const;

    // Returns the number of seconds left until show start, in microseconds
    int64_t get_time_until_start_usec() const;

    // Returns the number of seconds left until show start, in seconds
    float get_time_until_start_sec() const;

    // Returns the number of seconds left until the time when we should take off
    float get_time_until_takeoff_sec() const;

    // Returns the number of seconds left until the time when we should land
    float get_time_until_landing_sec() const;

    // Handles a MAVLink message forwarded to the drone show manager by the central MAVLink handler
    bool handle_message(const mavlink_message_t& msg);

    // Asks the drone show manager to schedule a start as soon as possible if
    // the show is not running yet, assuming that the signal was sent from the
    // remote controller.
    void handle_rc_start_switch();

    // Returns whether the drone has been authorized to start automatically by the user
    bool has_authorization_to_start() const;

    // Returns whether the show origin was set explicitly by the user
    bool has_explicit_show_origin_set_by_user() const;

    // Returns whether the show orientation was set explicitly by the user
    bool has_explicit_show_orientation_set_by_user() const;

    // Returns whether a scheduled start time was determined or set by the user for the show
    bool has_scheduled_start_time() const {
        return _start_time_usec > 0;
    }

    // Returns whether a valid takeoff time was determined for the show
    bool has_valid_takeoff_time() const {
        return _takeoff_time_sec >= 0 && _landing_time_sec > _takeoff_time_sec;
    }

    // Returns whether a show file was identified and loaded at boot time
    bool loaded_show_data_successfully() const;

    // Notifies the drone show manager that the drone show mode was initialized
    void notify_drone_show_mode_initialized();

    // Notifies the drone show manager that the drone show mode exited
    void notify_drone_show_mode_exited();

    // Notifies the drone show manager that the drone show mode has entered the given execution stage
    void notify_drone_show_mode_entered_stage(DroneShowModeStage stage);

    // Notifies the drone show manager that the drone has landed after the show
    void notify_landed();

    // Notifies the drone show manager that the takeoff is about to take place; yaw angle is in radians
    void notify_takeoff(const Location& loc, float yaw);

    // Handler for the MAVLink CMD_USER1 message that allows the user to reload _or_ clear the show
    bool reload_or_clear_show(bool do_clear);

    // Asks the drone show manager to reload the show file from the storage
    bool reload_show_from_storage();

    // Sends a drone show status message (wrapped in a DATA16 packet) on the given MAVLink channel
    void send_drone_show_status(const mavlink_channel_t chan) const;

    // Returns whether the drone should switch to show mode automatically
    // after boot if there is no RC input
    bool should_switch_to_show_mode_at_boot() const;

    // Asks the drone show manager to schedule a start as soon as possible if
    // the show is not running yet
    void start_if_not_running();

    // Asks the drone show manager to cancel the show as soon as possible if
    // the show is running yet
    void stop_if_running();

    // Updates the state of the LED light on the drone and performs any additional
    // tasks that have to be performed regularly (such as checking for changes
    // in parameter values).
    void update();

    static const struct AP_Param::GroupInfo var_info[];

    // Takeoff altitude; the drone attempts to take off to this altitude before
    // starting navigation
    static constexpr float TAKEOFF_ALTITUDE_METERS = 2.5f;

    // Takeoff speed; the drone attempts to take off with this vertical speed
    static constexpr float TAKEOFF_SPEED_METERS_PER_SEC = 1.0f;

    // Landing altitude; the drone attempts to navigate to this altitude before
    // starting landing at the end
    static constexpr float LANDING_ALTITUDE_METERS = 3.0f;

private:
    // Structure holding all the parameters settable by the user
    struct {
        // Start time in GPS time of week (seconds), as set by the user in a parameter
        AP_Int32 start_time_gps_sec;

        // Latitude of drone show coordinate system, in 1e-7 degrees, as set in the parameters by the user
        AP_Int32 origin_lat;

        // Longitude of drone show coordinate system, in 1e-7 degrees, as set in the parameters by the user
        AP_Int32 origin_lng;

        // Orientation of drone show coordinate system, in degrees, as set in the parameters by the user
        AP_Float orientation_deg;

        // Whether the drone has been authorized to start
        AP_Int8 authorized_to_start;

        // Whether the drone should boot in show mode
        AP_Int8 boot_in_show_mode;

        struct {
            // Specifies where the a given LED light channel of the show should be sent
            AP_Int8 type;

            AP_Int8 channel;
            
            // Specifies the number of LEDs on a LED strip at the given channel; used only for NeoPixel or ProfiLED types
            AP_Int8 count;
        } led_specs[1];
    } _params;

#if CONFIG_HAL_BOARD == HAL_BOARD_SITL
    // Socket that we use to transmit the current status of the RGB led to an
    // external process for visualization purposes
    SocketAPM _sock_rgb;

    // Stores whether the RGB LED socket is open
    bool _sock_rgb_open;
#endif

    // Memory area holding the entire show file loaded from the storage
    uint8_t* _show_data;

    struct sb_trajectory_s* _trajectory;
    struct sb_trajectory_player_s* _trajectory_player;
    bool _trajectory_valid;

    struct sb_light_program_s* _light_program;
    struct sb_light_player_s* _light_player;
    bool _light_program_valid;

    // Latitude of drone show coordinate system, in degrees
    int32_t _origin_lat;

    // Longitude of drone show coordinate system, in degrees
    int32_t _origin_lng;

    // Orientation of drone show coordinate system, in degrees, as set in the parameters by the user
    float _orientation_deg;

    // Reason why the start time was set to the current value; used to decide whether
    // it should be cleared when the drone show mode is restarted
    StartTimeSource _start_time_source;

    // Start time of the show, in microseconds, as a UNIX timestamp, zero if unset.
    uint64_t _start_time_usec;

    // Time when we need to take off, relative to the start of the show, in seconds
    float _takeoff_time_sec;

    // Time when we need to land relative to the start of the show, in seconds
    float _landing_time_sec;

    // Time when the user requested a light signal, in milliseconds; zero if
    // no signal was requested.
    uint32_t _light_signal_started_at_msec;

    // Current execution stage of the drone show mode. This is pushed here from
    // the drone show mode when the stage changes in the mode.
    DroneShowModeStage _stage_in_drone_show_mode;

    // Total duration of the show, in seconds
    float _total_duration_sec;

    // Flag that is set to true if the user has instructed the drone show manager
    // to cancel the show as soon as possible. This is checked regularly by
    // mode_drone_show.cpp
    bool _cancel_requested;

    // Factory object that can create RGBLed instances that the drone show manager will control
    DroneShowLEDFactory* _rgb_led_factory;

    // RGB led that the drone show manager controls
    DroneShowLED* _rgb_led;

    // Last RGB color that was sent to the RGB led
    struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } _last_rgb_led_color;

    // Timestamp that defines whether the RC start switch is blocked (and if so, until when)
    uint32_t _rc_start_switch_blocked_until;

    // Checks whether there were any changes in the parameters relevant to the
    // execution of the drone show. This has to be called regularly from update()
    void _check_changes_in_parameters();

    // Checks whether the radio is in failsafe state and blocks the RC start
    // switch until the radio returns from failsafe and at least one second
    // has passed
    void _check_radio_failsafe();

    // Clears the start time of the drone show after a successful landing
    void _clear_start_time_after_landing();

    // Clears the start time of the drone show if it was set by the user with the RC switch
    void _clear_start_time_if_set_by_switch();

    // Handles a MAVLink LED_CONTROL message from the ground station.
    bool _handle_led_control_message(const mavlink_message_t& msg);

    // Returns whether the GPS fix of the drone is good enough so we can trust
    // that it has accurate tiem information.
    bool _is_gps_time_ok() const;

    bool _load_show_file_from_storage();
    void _set_light_program_and_take_ownership(struct sb_light_program_s *value);
    void _set_trajectory_and_take_ownership(struct sb_trajectory_s *value);
    void _set_show_data_and_take_ownership(uint8_t *value);

    // Updates the state of the LED light on the drone. This has to be called
    // regularly at 25 Hz
    void _update_lights();

    // Updates the RGB LED instance that is used as an output. Note that this
    // function updates the identity of the RGB LED object based on the current
    // servo channel settings, _not_ the state of the LED itself
    void _update_rgb_led_instance();

#if CONFIG_HAL_BOARD == HAL_BOARD_SITL
    bool _open_rgb_led_socket();
#endif
};
