#include "AboutDialog.h"
#include <version.h>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    ui.labelVersion->setText("v" VER_PRODUCTVERSION_STR);

}
