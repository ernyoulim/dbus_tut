#include <gio/gio.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

static GDBusConnection *connection;

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='com.example.MiniNM'>"
  "    <method name='ListInterfaces'>"
  "      <arg type='a{ss}' name='interfaces' direction='out'/>"
  "    </method>"
  "    <method name='GetInterfaceState'>"
  "      <arg type='s' name='iface' direction='in'/>"
  "      <arg type='s' name='state' direction='out'/>"
  "    </method>"
  "    <signal name='InterfaceChanged'>"
  "      <arg type='s' name='iface'/>"
  "      <arg type='s' name='state'/>"
  "    </signal>"
  "  </interface>"
  "</node>";

static gchar* get_iface_state(const char *iface) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
        return g_strdup("UNKNOWN");

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_name && strcmp(ifa->ifa_name, iface) == 0) {
            gboolean up = (ifa->ifa_flags & IFF_UP);
            freeifaddrs(ifaddr);
            return g_strdup(up ? "UP" : "DOWN");
        }
    }

    freeifaddrs(ifaddr);
    return g_strdup("NOT_FOUND");
}

static GVariant* list_interfaces() {
    struct ifaddrs *ifaddr, *ifa;
    GVariantBuilder builder;
    GHashTable *seen;

    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{ss}"));
    seen = g_hash_table_new(g_str_hash, g_str_equal);

    if (getifaddrs(&ifaddr) == -1)
        return g_variant_new("(a{ss})", NULL);

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_name || !ifa->ifa_addr)
            continue;

        if (g_hash_table_contains(seen, ifa->ifa_name))
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            char addr[INET_ADDRSTRLEN];
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;

            inet_ntop(AF_INET, &sa->sin_addr, addr, sizeof(addr));

            g_variant_builder_add(&builder, "{ss}", ifa->ifa_name, addr);
            g_hash_table_add(seen, g_strdup(ifa->ifa_name));
        }
    }

    freeifaddrs(ifaddr);
    g_hash_table_destroy(seen);

    return g_variant_new("(a{ss})", &builder);
}

static void emit_signal(const char *iface, const char *state) {
    g_dbus_connection_emit_signal(
        connection,
        NULL,
        "/com/example/MiniNM",
        "com.example.MiniNM",
        "InterfaceChanged",
        g_variant_new("(ss)", iface, state),
        NULL);
}

static void handle_method_call(
    GDBusConnection *conn,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data)
{
    if (g_strcmp0(method_name, "ListInterfaces") == 0) {
        g_dbus_method_invocation_return_value(invocation, list_interfaces());
    }
    else if (g_strcmp0(method_name, "GetInterfaceState") == 0) {
        const gchar *iface;
        g_variant_get(parameters, "(&s)", &iface);
        gchar *state = get_iface_state(iface);

        g_dbus_method_invocation_return_value(
            invocation,
            g_variant_new("(s)", state));

        g_free(state);
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call,
    NULL,
    NULL
};

/* Simple polling to detect state changes */
static gboolean monitor_interfaces(gpointer user_data) {
    static GHashTable *prev_states = NULL;
    if (!prev_states)
        prev_states = g_hash_table_new(g_str_hash, g_str_equal);

    struct ifaddrs *ifaddr, *ifa;
    getifaddrs(&ifaddr);

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_name) continue;

        gchar *state = get_iface_state(ifa->ifa_name);
        gchar *old = g_hash_table_lookup(prev_states, ifa->ifa_name);

        if (!old || g_strcmp0(old, state) != 0) {
            emit_signal(ifa->ifa_name, state);
            g_hash_table_replace(prev_states,
                                 g_strdup(ifa->ifa_name),
                                 g_strdup(state));
        }
        g_free(state);
    }

    freeifaddrs(ifaddr);
    return TRUE; // continue polling
}

int main() {
    GError *error = NULL;
    GMainLoop *loop;
    GDBusNodeInfo *introspection_data;

    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);

    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

	g_bus_own_name_on_connection(
	    connection,
	    "com.example.MiniNM",
	    G_BUS_NAME_OWNER_FLAGS_NONE,
	    NULL,
	    NULL,
	    NULL,
	    NULL
	);
    g_dbus_connection_register_object(
        connection,
        "/com/example/MiniNM",
        introspection_data->interfaces[0],
        &interface_vtable,
        NULL, NULL, &error);

    g_print("Mini Network Manager running...\n");

    g_timeout_add_seconds(2, monitor_interfaces, NULL);

    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
}
