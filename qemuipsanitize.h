/* QEMU local-IP-address fixer.

   Qemu guest OSs will think 127.0.0.1 is itself, and not the host OS.
   Fix that!

 */

const char* sanitize_ipaddress(const char*addr);
char* sanitize_mirror(const char*addr);

