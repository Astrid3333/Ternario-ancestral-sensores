# Ternary Ancestral Kernel v2 — Userspace

Un kernel ternario que **corre por encima de Linux**. Ultra-lite (48KB), usa Linux de sustento para todo lo complejo (procesos, memoria, filesystem, I/O, red).

## Qué es esto

```
┌─────────────────────────────────────────────┐
│  TAK (Ternary Ancestral Kernel)            │  ← Este binario (48KB)
│  Shell + Scheduler + Memory + FS           │
├─────────────────────────────────────────────┤
│  Linux Kernel                              │  ← El sustento real
│  Processes, Memory, Files, Network, I/O    │
└─────────────────────────────────────────────┘
```

- **Tamaño:** 48KB binario
- **RAM:** ~1MB en ejecución
- **Dependencias:** gcc, libc (en cualquier Linux)
- **Archivos:** 4 archivos C + 1 header

## Compilación

```bash
make          # Compila → ./tak (48KB)
make install  # Instala en /usr/local/bin/tak
```

Requisitos: `gcc`, `libc-dev` (viene con casi todo Linux)

## Uso

```bash
./tak                    # Modo interactivo
./tak -c "trit 42"      # Ejecutar un comando
echo -e "ps\ncal" | ./tak  # Pipe
```

## Comandos del Shell

### Procesos (Linux debajo)
```
ps              Listar procesos TAK + Linux PIDs
fork <cmd>      Fork+exec un comando Linux (ls, cat, gcc, etc)
kill <pid>      Matar un proceso TAK
```

### Memoria (Base 60)
```
mem             Estado de memoria con block map
malloc <bytes>  Asignar bloque ternario
free <block>    Liberar bloque
```

### Filesystem Quipu (real directory)
```
fs                              Listar archivos
touch <nombre> <datos>          Crear archivo
cat <nombre>                    Leer archivo
rm <nombre>                     Borrar archivo
```

### Utilidades ternarias
```
cal                             Calendario Maya (Tzolkin/Haab)
trit <n>                        Convertir a ternario {-1,0,+1}
b60 <n>                         Convertir a Base 60
```

### Sistema
```
whoami                          Usuario actual
uname                           Info del sistema
uptime                          Tiempo activo
neofetch                        Info ternaria del sistema
clear                           Limpiar pantalla
halt                            Salir
```

## Ejemplo de uso

```
tak@mayan:2/2$ trit 42
  42 → ++-0

tak@mayan:3/3$ b60 3600
  3600 → 60:0 (Base 60)

tak@mayan:4/4$ fork ls /tmp
  ✓ Created TAK PID 1 (Linux PID 12345): ls /tmp

tak@mayan:5/5$ ps
  PID   STATE     PRI  LINUX_PID  NAME
  0     ACTIVE    0    12340      init
  1     ACTIVE    0    12345      ls

tak@mayan:6/6$ touch test.dat "hola mundo"
  ✓ Created 'test.dat' (10 bytes)

tak@mayan:7/7$ cat test.dat
  hola mundo

tak@mayan:8/8$ cal
  Global tick:   8
  Tzolkin day:   8 / 260
  Haab day:      8 / 365
  System load:   LOW

tak@mayan:9/9$ mem
  Active blocks:  5 / 60
  Block map:
  ████████████████████████████████·····························
```

## Arquitectura

### Módulos
| Módulo | Archivo | Qué hace |
|--------|---------|----------|
| Core | `src/tak.c` | Main, shell, comandos |
| Scheduler | `src/scheduler.c` | Wraps Linux fork/exec con IDs Maya |
| Filesystem | `src/filesystem.c` | Quipu sobre directorio real |
| Memory | `src/memory.c` | malloc wrapper con tracking Base 60 |
| Header | `include/ternary.h` | Tipos ternarios, constantes |

### Cómo funciona cada subsistema

**Scheduler:** Cada "proceso TAK" tiene un PID Maya (Tzolkin/Haab). Detrás, es un proceso Linux real (fork/exec). Linux hace el scheduling real; TAK solo trackea estado.

**Filesystem:** Los archivos Quipu viven en `~/.tak/quipu/`. Son archivos normales del filesystem Linux. TAK agrega nombres de 8 caracteres y permisos ternarios.

**Memoria:** `malloc` normal de libc. TAK trackea 60 "bloques" con colores (tipo) y dueños. Es un wrapper, no un allocator custom.

**Ternary logic:** Operaciones {-1, 0, +1} empaquetadas en 5 bits (3 trits). Compresión de datos para sensores IoT.

## Por qué userspace

| Aspecto | Bare-metal | Userspace (este) |
|---------|-----------|-----------------|
| Compilar | cross-compiler i686-elf | gcc normal |
| Ejecutar | QEMU | Cualquier Linux |
| Memoria | 3.5KB manual | malloc ilimitado |
| Procesos | 33 max, cooperative | Ilimitados, preemptive |
| FS | 33 archivos | Ilimitado |
| Drivers | Manual | Linux los da |
| Compilar en | 10 min setup | `make` y listo |

## Futuro

- [ ] Network stack ternario (wrapper sobre sockets)
- [ ] Ternary compression CLI (`tak compress <file>`)
- [ ] IoT sensor daemon mode (`tak --daemon`)
- [ ] GUI ternaria (wrapper sobre X11/Wayland)
- [ ] Package manager ternario

## Licencia

MIT
