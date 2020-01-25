#include "BackupPage.h"

#include <sstream>

#include <QCheckBox>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>

#include <retroshare/rspeers.h>
#include <services/p3idservice.h>

#include <util/rsdebug.h>

RS_SET_CONTEXT_DEBUG_LEVEL(4)

static const std::string rsBackupIdentityFiled = "rsBackupId";

BackupPage::BackupPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags), _currentCategory(nullptr), _currentItem(nullptr)
{
    Dbg3() << __PRETTY_FUNCTION__ << std::endl;

    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);

    // connect ui
    // expert check box
    QObject::connect(ui.cb_expert,   SIGNAL(stateChanged(int)),      this, SLOT(on_cb_expert_stateChanged(int)       ));
    // categories & itmes lists
    QObject::connect(ui.lw_category, SIGNAL(currentRowChanged(int)), this, SLOT(on_lw_category_currentRowChanged (int)));
    QObject::connect(ui.lw_item,     SIGNAL(currentRowChanged(int)), this, SLOT(on_lw_item_currentRowChanged(int)    ));
    // buttons
    QObject::connect(ui.pb_load,     SIGNAL(released()),             this, SLOT(on_pb_load_released()));
    QObject::connect(ui.pb_new,      SIGNAL(released()),             this, SLOT(on_pb_new_released() ));

    init();
}

void BackupPage::init()
{
    // functions
    auto loadIdentity = [](std::string idStr) {
        RsGxsId id = RsGxsId(idStr);

        p3IdService *ids = dynamic_cast<p3IdService*>(rsIdentity);
        if (ids == nullptr)
            return std::string("Error");

        rsBackupId rsbi;
        rsIdentity->getIdDetails(id, rsbi.idd);
        RsTlvPrivateRSAKey secKey;
        ids->getPrivateKey(id, secKey);
        rsbi.secKey = secKey;

        RsGenericSerializer::SerializeContext ctx;
        {
            RsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);
            RsTypeSerializer::serial_process<rsBackupId>(j, ctx, rsbi, rsBackupIdentityFiled);
        }

        std::stringstream ss;
        ss << ctx.mJson;
        return ss.str();
    };

//    auto createIdentity = [](std::string input) {
//        (void) input;
//        return true;
//    };

    // Identities
    struct backUpCaterogy identitiesSigned;
    identitiesSigned.name = QObject::tr("Identities (signed)");
    identitiesSigned.load = loadIdentity;
    identitiesSigned.createNew = nullptr;
    {
        std::vector<RsGxsId> ids;
        if(rsIdentity->getOwnSignedIds(ids)) {
            for(auto id : ids) {
                struct backUpItem a;
                a.value = id.toStdString();

                // lookup name
                RsIdentityDetails det;
                if(rsIdentity->getIdDetails(id, det)) {
                    std::string txt = det.mNickname + "<" + id.toStdString() +  ">";
                    a.displayText = QString::fromStdString(txt);
                } else {
                    a.displayText = QString::fromStdString(id.toStdString());
                }

                identitiesSigned.items.push_back(a);
            }
        }
    }
    _categories.push_back(identitiesSigned);

    struct backUpCaterogy identitiesAnon;
    identitiesAnon.name = QObject::tr("Identities (unsigned/anonymous)");
    identitiesAnon.load = loadIdentity;
    identitiesAnon.createNew = nullptr;
    {
        std::vector<RsGxsId> ids;
        if(rsIdentity->getOwnPseudonimousIds(ids)) {
            for(auto id : ids) {
                struct backUpItem a;
                a.value = id.toStdString();

                // lookup name
                RsIdentityDetails det;
                if(rsIdentity->getIdDetails(id, det)) {
                    std::string txt = det.mNickname + "<" + id.toStdString() +  ">";
                    a.displayText = QString::fromStdString(txt);
                } else {
                    a.displayText = QString::fromStdString(id.toStdString());
                }

                identitiesAnon.items.push_back(a);
            }
        }
    }
    _categories.push_back(identitiesAnon);

    // fill categories
    Dbg4() << __PRETTY_FUNCTION__ << " len(_categories) = " << _categories.length() << std::endl;
    for (auto cat : _categories) {
        ui.lw_category->addItem(cat.name);
    }
}

void BackupPage::on_cb_expert_stateChanged(int arg1)
{
    Dbg3() << __PRETTY_FUNCTION__ << std::endl;

    ui.gb_expert->setEnabled(arg1);
}

void BackupPage::on_lw_category_currentRowChanged(int currentRow)
{
    Dbg3() << __PRETTY_FUNCTION__ << std::endl;
    Dbg4() << __PRETTY_FUNCTION__ << " currentRow=" << currentRow << std::endl;

    if (currentRow < 0)
        ui.lw_category->clear();
        return;

    _currentItem = nullptr;
    _currentCategory = &_categories[currentRow];

    ui.lw_item->clear();
    ui.pte_inout->clear();

    Dbg4() << __PRETTY_FUNCTION__ << " len(_currentCategory) = " << _currentCategory->items.length() << std::endl;
    for (auto item : _currentCategory->items) {
        ui.lw_item->addItem(item.displayText);
    }
}

void BackupPage::on_lw_item_currentRowChanged(int currentRow)
{
    Dbg3() << __PRETTY_FUNCTION__ << std::endl;
    Dbg4() << __PRETTY_FUNCTION__ << " currentRow=" << currentRow << std::endl;

    if (currentRow < 0)
        ui.lw_item->clear();
        return;

    _currentItem = &_currentCategory->items[currentRow];
    emit on_pb_load_released();
}

void BackupPage::on_pb_load_released()
{
    Dbg3() << __PRETTY_FUNCTION__ << std::endl;

    if(_currentCategory->load == nullptr)
        // TODO inform user
        return;

    std::string s = _currentCategory->load(_currentItem->value);
    ui.pte_inout->setPlainText(QString::fromStdString(s));
}

void BackupPage::on_pb_new_released()
{
    Dbg3() << __PRETTY_FUNCTION__ << std::endl;

    if(_currentCategory->createNew == nullptr)
        // TODO inform user
        return;

    QString s = ui.pte_inout->toPlainText();
    bool success = _currentCategory->createNew(s.toStdString());
    if (success)
        ui.pte_inout->clear();
    // TODO tell user about outcome
}

