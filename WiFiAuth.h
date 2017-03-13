/* WiFiAuth.h
  
   Definintion of constants, related to establishing a connection to a Wi-Fi
   network. The values below, by default, are populated with placeholders.
   These (of course) should be updated, to contain the values specific to the
   network that the node will be connecting to.

   NOTE: *do not* commit any versions of this file, that contain actual
   credentials. That would be bad.

   To assure that changes to this file aren't tracked, once the repo has been
   initially cloned to a machine, use the following command:

     git update-index --assume-unchanged WiFiAuth.h

   If, for some reason, a change to this file needs to be made and committed
   (without credentials, of course), then use the following command to
   (temporarily) track changes to it again:

     git update-index --no-assume-unchanged WiFiAuth.h

   Once the necessary changes are committed and pushed, promptly stop tracking
   changes to the file, again, using the first command.

   More details, on this option, can be found in the Git documentation.

     https://git-scm.com/docs/git-update-index#git-update-index---no-assume-unchanged

 */

const char* SSID = "enter-SSID-here";
const char* PASSWORD = "enter-password-here";

