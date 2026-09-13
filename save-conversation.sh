#!/bin/bash
# Guarda la conversación actual como archivo markdown
# Uso: ./save-conversation.sh "título de la sesión"

TITLE="${1:-conversacion-$(date +%Y-%m-%d_%H%M)}"
SESSION_DIR="$(dirname "$0")"
FILE="${SESSION_DIR}/${TITLE}.md"

cat > "$FILE" << EOF
# ${TITLE}
## $(date '+%Y-%m-%d %H:%M')

$(cat /dev/stdin)
EOF

echo "Guardado en: $FILE"
