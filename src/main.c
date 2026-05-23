#include "mongoose.h"
#include "cpu_status.h"
#include "system_log.h"
#include "my_certs.h"

#include <stdbool.h>

struct mg_mgr mgr;
static int watchdog_timer = 0;
static struct mg_connection *s_mqtt_con = NULL;
static bool s_mqtt_connected;
static float cpu_usage;
static uint32_t ram_usage;

static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    (void)ev_data;

    if (ev == MG_EV_OPEN) {
        write_sys_log("HTTP", "Connected");
    }
    else if (ev == MG_EV_HTTP_MSG) {
        char response[256];
        snprintf(response, sizeof(response), "{\n  \"mqtt_status\": \"%s\",\n  \"cpu\": \"%.2f%%\",\n  \"ram_kb\": \"%u kB\"\n}\n",
                 s_mqtt_connected ? "CONNECTED" : "DISCONNECTED", cpu_usage, ram_usage);
        mg_http_reply(c, 200, "Content-Type: application/json\r\nConnection: close\r\n", "%s", response);
    }
}

static void mqtt_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    (void)c;

    if (ev == MG_EV_MQTT_OPEN) {
        s_mqtt_connected = true;
        watchdog_timer = 0;
        write_sys_log("MQTT", "Connected");
    } else if (ev == MG_EV_READ) {
        watchdog_timer = 0;
    } else if (ev == MG_EV_ERROR) {
        // printf("MQTT    Error: %s\r\n", (char *)ev_data);
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "Error: %s", (char *)ev_data);
        write_sys_log("MQTT", err_msg);
    } else if (ev == MG_EV_CLOSE) {
        s_mqtt_connected = false;
        s_mqtt_con = NULL;
        write_sys_log("MQTT", "Disconnected");
    }
}

static void timer_cb(void *arg) {
    static int count = 0;
    static int log_manage_count = 0;
    (void)arg;

    count++;
    log_manage_count++;

    if (s_mqtt_connected != false && s_mqtt_con != NULL) {
        watchdog_timer++;
        if (watchdog_timer > 8) {
            write_sys_log("MQTT", "Watchdog Timeout");
            s_mqtt_con->is_closing = 1;
            watchdog_timer = 0;
        }
    }

    if (s_mqtt_con == NULL) {
        struct mg_mqtt_opts opts = {
            .clean = true,
            .version = 4,
            .keepalive = 5,
        };
        s_mqtt_con = mg_mqtt_connect(&mgr, mqtt_url, &opts, mqtt_ev_handler, NULL); 
        if (s_mqtt_con != NULL) {
            struct mg_tls_opts tls_opts = {
                .ca = mg_str_s(ca_cert_pem),
                .name = mg_str("192.168.1.2")
            };
            mg_tls_init(s_mqtt_con, &tls_opts);
        }
    }

    if (count % 5 == 0) {
        read_cpu_usage(&cpu_usage);
        read_ram_usage(&ram_usage);

        char payload[128];
        snprintf(payload, sizeof(payload), "{\"cpu_percent\": %.2f, \"ram_kb\": %u}", cpu_usage, ram_usage);
        write_sys_log("BENCHMARK", payload);
        if (s_mqtt_connected != false && s_mqtt_con != NULL) {
            struct mg_mqtt_opts pub_opts;
            memset(&pub_opts, 0, sizeof(pub_opts));
            pub_opts.topic = mg_str("anyka/board/benchmark");
            pub_opts.message = mg_str(payload);
            pub_opts.qos = 1;

            mg_mqtt_pub(s_mqtt_con, &pub_opts);
            // printf("BENCHMARK   Public sucessfully %s\r\n", payload);
        }
    }

    if (log_manage_count >= (3600*24)) {
        manage_sys_log();
        log_manage_count = 0;
    }
}

int main(void){
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, http_url, http_ev_handler, NULL);
    mg_timer_add(&mgr, 1000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, timer_cb, NULL);

    while(1){
        mg_mgr_poll(&mgr, 50);
    }

    mg_mgr_free(&mgr);
    return 0;
}

