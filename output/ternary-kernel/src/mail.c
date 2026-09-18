/**
 * mail.c — Cliente de correo para kernel ternario ancestral
 *
 * SMTP para enviar, POP3 para recibir
 */

#include "../include/ternary.h"

#define MAIL_MAX_ACCOUNTS 4
#define MAIL_MAX_EMAILS   16
#define MAIL_MAX_ADDR     64
#define MAIL_MAX_SUBJ     64
#define MAIL_MAX_BODY     512

typedef struct {
    char     email[MAIL_MAX_ADDR];
    char     smtp_host[MAIL_MAX_ADDR];
    char     pop3_host[MAIL_MAX_ADDR];
    uint16_t smtp_port;
    uint16_t pop3_port;
    char     username[MAIL_MAX_ADDR];
    char     password[32];
    uint8_t  active;
} mail_account_t;

typedef struct {
    char     from[MAIL_MAX_ADDR];
    char     to[MAIL_MAX_ADDR];
    char     subject[MAIL_MAX_SUBJ];
    char     body[MAIL_MAX_BODY];
    uint8_t  read;
    uint8_t  in_use;
} mail_email_t;

static mail_account_t accounts[MAIL_MAX_ACCOUNTS];
static mail_email_t emails[MAIL_MAX_EMAILS];
static uint8_t n_accounts = 0;
static uint8_t n_emails = 0;

// Initialize mail client
void mail_init(void) {
    vga_puts("[MAIL] Email client initialized\n");
}

// Add account
int8_t mail_add_account(const char* email, const char* smtp_host, 
                        const char* pop3_host, uint16_t smtp_port, uint16_t pop3_port) {
    if (n_accounts >= MAIL_MAX_ACCOUNTS) {
        vga_puts("[MAIL] Too many accounts\n");
        return -1;
    }
    
    strcpy_t(accounts[n_accounts].email, email);
    strcpy_t(accounts[n_accounts].smtp_host, smtp_host);
    strcpy_t(accounts[n_accounts].pop3_host, pop3_host);
    accounts[n_accounts].smtp_port = smtp_port;
    accounts[n_accounts].pop3_port = pop3_port;
    accounts[n_accounts].active = 1;
    n_accounts++;
    
    vga_puts("[MAIL] Account added: ");
    vga_puts(email);
    vga_puts("\n");
    
    return 0;
}

// Send email (SMTP)
int8_t mail_send(const char* to, const char* subject, const char* body) {
    if (n_accounts == 0) {
        vga_puts("[MAIL] No accounts configured\n");
        return -1;
    }
    
    vga_puts("[MAIL] Sending email...\n");
    
    // Resolve SMTP server
    uint32_t smtp_ip = dns_resolve(accounts[0].smtp_host);
    if (smtp_ip == 0) {
        vga_puts("[MAIL] Cannot resolve SMTP server\n");
        return -1;
    }
    
    // TCP connect to SMTP
    int8_t sock = tcp_connect(smtp_ip, accounts[0].smtp_port);
    if (sock < 0) {
        vga_puts("[MAIL] SMTP connection failed\n");
        return -1;
    }
    
    // Receive greeting
    uint8_t buf[1024];
    tcp_recv(sock, buf, sizeof(buf));
    
    // Send HELO
    const char* helo = "HELO tritos\r\n";
    tcp_send(sock, (uint8_t*)helo, strlen_t(helo));
    tcp_recv(sock, buf, sizeof(buf));
    
    // Send MAIL FROM
    const char* mail_from = "MAIL FROM:<";
    tcp_send(sock, (uint8_t*)mail_from, strlen_t(mail_from));
    tcp_send(sock, (uint8_t*)accounts[0].email, strlen_t(accounts[0].email));
    tcp_send(sock, (uint8_t*)">\r\n", 3);
    tcp_recv(sock, buf, sizeof(buf));
    
    // Send RCPT TO
    const char* rcpt_to = "RCPT TO:<";
    tcp_send(sock, (uint8_t*)rcpt_to, strlen_t(rcpt_to));
    tcp_send(sock, (uint8_t*)to, strlen_t(to));
    tcp_send(sock, (uint8_t*)">\r\n", 3);
    tcp_recv(sock, buf, sizeof(buf));
    
    // Send DATA
    const char* data_cmd = "DATA\r\n";
    tcp_send(sock, (uint8_t*)data_cmd, strlen_t(data_cmd));
    tcp_recv(sock, buf, sizeof(buf));
    
    // Send headers
    tcp_send(sock, (uint8_t*)"From: ", 6);
    tcp_send(sock, (uint8_t*)accounts[0].email, strlen_t(accounts[0].email));
    tcp_send(sock, (uint8_t*)"\r\n", 2);
    
    tcp_send(sock, (uint8_t*)"To: ", 4);
    tcp_send(sock, (uint8_t*)to, strlen_t(to));
    tcp_send(sock, (uint8_t*)"\r\n", 2);
    
    tcp_send(sock, (uint8_t*)"Subject: ", 9);
    tcp_send(sock, (uint8_t*)subject, strlen_t(subject));
    tcp_send(sock, (uint8_t*)"\r\n", 2);
    
    tcp_send(sock, (uint8_t*)"\r\n", 2);
    
    // Send body
    tcp_send(sock, (uint8_t*)body, strlen_t(body));
    tcp_send(sock, (uint8_t*)"\r\n.\r\n", 5);
    tcp_recv(sock, buf, sizeof(buf));
    
    // QUIT
    const char* quit = "QUIT\r\n";
    tcp_send(sock, (uint8_t*)quit, strlen_t(quit));
    tcp_recv(sock, buf, sizeof(buf));
    
    tcp_close(sock);
    
    vga_set_color(0x0A, 0);
    vga_puts("[MAIL] Email sent to ");
    vga_puts(to);
    vga_puts("\n");
    vga_set_color(0x07, 0);
    
    return 0;
}

// Receive emails (POP3) - simulated
int8_t mail_receive(void) {
    if (n_accounts == 0) {
        vga_puts("[MAIL] No accounts configured\n");
        return -1;
    }
    
    vga_puts("[MAIL] Checking for new emails...\n");
    
    // Simulate receiving emails
    if (n_emails < MAIL_MAX_EMAILS) {
        strcpy_t(emails[n_emails].from, "admin@tritos.org");
        strcpy_t(emails[n_emails].to, accounts[0].email);
        strcpy_t(emails[n_emails].subject, "Welcome to Tritos");
        strcpy_t(emails[n_emails].body, "Welcome to the Ternary Ancestral Operating System!");
        emails[n_emails].read = 0;
        emails[n_emails].in_use = 1;
        n_emails++;
    }
    
    vga_puts("[MAIL] ");
    { char nb[4]; num_to_str(n_emails, nb); vga_puts(nb); }
    vga_puts(" emails in inbox\n");
    
    return 0;
}

// List emails
void mail_list(void) {
    vga_puts("\n  Inbox:\n\n");
    
    uint8_t count = 0;
    for (int i = 0; i < MAIL_MAX_EMAILS; i++) {
        if (emails[i].in_use) {
            vga_puts("  [");
            if (emails[i].read) {
                vga_puts(" ");
            } else {
                vga_set_color(0x0B, 0);
                vga_puts("N");
                vga_set_color(0x07, 0);
            }
            vga_puts("] From: ");
            vga_puts(emails[i].from);
            vga_puts("\n      Subject: ");
            vga_puts(emails[i].subject);
            vga_puts("\n");
            count++;
        }
    }
    
    if (count == 0) {
        vga_puts("  No emails\n");
    }
}

// Read email
void mail_read(uint8_t index) {
    if (index >= MAIL_MAX_EMAILS || !emails[index].in_use) {
        vga_puts("[MAIL] Invalid email\n");
        return;
    }
    
    emails[index].read = 1;
    
    vga_puts("\n  From: ");
    vga_puts(emails[index].from);
    vga_puts("\n  To: ");
    vga_puts(emails[index].to);
    vga_puts("\n  Subject: ");
    vga_puts(emails[index].subject);
    vga_puts("\n\n  ");
    vga_puts(emails[index].body);
    vga_puts("\n");
}

// Delete email
void mail_delete(uint8_t index) {
    if (index >= MAIL_MAX_EMAILS || !emails[index].in_use) {
        vga_puts("[MAIL] Invalid email\n");
        return;
    }
    
    emails[index].in_use = 0;
    n_emails--;
    vga_puts("[MAIL] Email deleted\n");
}

// List accounts
void mail_accounts(void) {
    vga_puts("\n  Email accounts:\n\n");
    
    for (int i = 0; i < n_accounts; i++) {
        vga_puts("  ");
        { char nb[2]; num_to_str(i, nb); vga_puts(nb); }
        vga_puts(". ");
        vga_puts(accounts[i].email);
        vga_puts("\n     SMTP: ");
        vga_puts(accounts[i].smtp_host);
        vga_puts(":");
        { char nb[8]; num_to_str(accounts[i].smtp_port, nb); vga_puts(nb); }
        vga_puts("\n     POP3: ");
        vga_puts(accounts[i].pop3_host);
        vga_puts(":");
        { char nb[8]; num_to_str(accounts[i].pop3_port, nb); vga_puts(nb); }
        vga_puts("\n");
    }
}

// Show mail status
void mail_status(void) {
    vga_puts("\n  Mail status:\n\n");
    
    vga_puts("  Accounts: ");
    { char nb[4]; num_to_str(n_accounts, nb); vga_puts(nb); }
    vga_puts("\n");
    
    uint8_t unread = 0;
    for (int i = 0; i < MAIL_MAX_EMAILS; i++) {
        if (emails[i].in_use && !emails[i].read) unread++;
    }
    
    vga_puts("  Unread: ");
    { char nb[4]; num_to_str(unread, nb); vga_puts(nb); }
    vga_puts("\n");
}
