#ifndef BACKUP_H
#define BACKUP_H

#include <retroshare/rsidentity.h>
#include <serialiser/rsserial.h>

#include <retroshare-gui/configpage.h>

#include "ui_BackupPage.h"

class RsPeers;

struct rsBackupId : RsSerializable
{
public:
    rsBackupId() {};
    rsBackupId(RsIdentityDetails &idd, RsTlvPrivateRSAKey &secKey) : idd(idd), secKey(secKey) {}

    // RsSerializable interface
public:
    void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
        RS_SERIAL_PROCESS(idd);
//        RS_SERIAL_PROCESS(secKey.);
        {
            RS_SERIAL_PROCESS(secKey.keyId);
            RS_SERIAL_PROCESS(secKey.keyFlags);
            RS_SERIAL_PROCESS(secKey.startTS);
            RS_SERIAL_PROCESS(secKey.endTS);

            RS_SERIAL_PROCESS(secKey.keyData.tlvtype);
            RS_SERIAL_PROCESS(secKey.keyData.bin_len);
            RsTypeSerializer::TlvMemBlock_proxy b(secKey.keyData.bin_data, secKey.keyData.bin_len);
            RsTypeSerializer::serial_process(j, ctx, b, "mData");
        }
    }

    RsIdentityDetails idd;
    RsTlvPrivateRSAKey secKey;
};

class BackupPage : public ConfigPage
{
    Q_OBJECT

public:
    BackupPage(QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~BackupPage() {}

    // ConfigPage interface
public:
    void load() {}
    QPixmap iconPixmap() const { return QPixmap(":/icons/settings/general.svg") ; };
    QString pageName() const { return QObject::tr("Backup") ; };
    QString helpText() const { return QObject::tr("Backup and restore important things") ; };

private slots:
    void on_cb_expert_stateChanged(int arg1);
    void on_lw_category_currentRowChanged(int currentRow);
    void on_lw_item_currentRowChanged(int currentRow);
    void on_pb_load_released();
    void on_pb_new_released();

private:

    void init();

    Ui::BackupPage ui;

    struct backUpItem {
        std::string value;
        QString displayText;
    };

    struct backUpCaterogy {
        QString name;
        QVector<backUpItem> items;
        std::function<std::string(std::string)> load;
        std::function<bool(std::string)> createNew;
    };

    QVector<struct backUpCaterogy> _categories;
    struct backUpCaterogy *_currentCategory;
    struct backUpItem *_currentItem;
};

#endif // BACKUP_H
