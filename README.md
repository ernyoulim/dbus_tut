# Mini Network Manager (D-Bus Tutorial)

A small educational project from OS class for demonstrating how to build a **D-Bus service in C using GLib/GDBus** that exposes Linux network interface information.

The service provides a simple API to:

* List network interfaces and their IPv4 addresses
* Query interface state
* Emit signals when interface state changes

---

# Features

* D-Bus service implemented in **C**
* Uses **GLib / GDBus**
* Lists interfaces with IP addresses
* Query interface state (UP/DOWN)
* Emits signals when interface state changes
* Demonstrates:

  * D-Bus method calls
  * D-Bus signals
  * GLib main loop
  * Linux networking APIs (`getifaddrs`)

---

# D-Bus API

Service name:

```
com.example.MiniNM
```

Object path:

```
/com/example/MiniNM
```

Interface:

```
com.example.MiniNM
```

---

## Methods

### ListInterfaces()

Returns all interfaces with IPv4 addresses.

```
a{ss}
```

Example:

```
{
  "lo": "127.0.0.1",
  "eth0": "192.168.1.10"
}
```

---

### GetInterfaceState(interface)

Returns interface state.

Input:

```
string interface
```

Output:

```
string state
```

Example:

```
"eth0" → "UP"
```

---

## Signals

### InterfaceChanged(interface, state)

Emitted when an interface changes state.

Example signal:

```
InterfaceChanged("eth0", "DOWN")
```

---

# Building

Requirements:

* GCC
* GLib / GIO development libraries

Install dependencies (Ubuntu/Debian):

```
sudo apt install build-essential libglib2.0-dev
```

Build the project:

```
make
```

---

# Running

Start the service:

```
./mini-nm
```

---

# Testing the API

List interfaces:

```
gdbus call --session \
  --dest com.example.MiniNM \
  --object-path /com/example/MiniNM \
  --method com.example.MiniNM.ListInterfaces
```

Example output:

```
({'lo': '127.0.0.1', 'eth0': '192.168.1.10'},)
```

---

Query interface state:

```
gdbus call --session \
  --dest com.example.MiniNM \
  --object-path /com/example/MiniNM \
  --method com.example.MiniNM.GetInterfaceState "eth0"
```

Example output:

```
('UP',)
```

---

# Monitoring Signals

You can observe signals using:

```
dbus-monitor "interface='com.example.MiniNM'"
```

Then change interface state:

```
sudo ip link set eth0 down
sudo ip link set eth0 up
```

You should see:

```
InterfaceChanged("eth0", "DOWN")
InterfaceChanged("eth0", "UP")
```
