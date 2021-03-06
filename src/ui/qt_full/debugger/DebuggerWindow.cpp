#include <QtCore/QSettings>
#include <thread>

#include "core/Cpu.h"
#include "core/Interrupt.h"
#include "core/Memory.h"

#include "../SettingsConstants.h"
#include "../UiUtils.h"
#include "AddressDialog.h"
#include "DebuggerWindow.h"
#include "DisassemblyModel.h"
#include "MemoryModel.h"
#include "ui_DebuggerWindow.h"


DebuggerWindow::DebuggerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DebuggerWindow),
    cpu(NULL),
    interrupt(NULL),
    memory(NULL),
    currentSp(0),
    debuggingEnabled(false),
    singleStep(false),
    runToAddress(0xFFFF),
    disassemblyModel(new DisassemblyModel(palette(), this)),
    memoryModel(new MemoryModel(this))
{
    ui->setupUi(this);
    ui->actionToggleDebugging->setChecked(debuggingEnabled);
    SlotToggleDebugging(debuggingEnabled);
    ui->menuView->addAction(ui->dockCallStack->toggleViewAction());
    ui->menuView->addAction(ui->dockMemory->toggleViewAction());
    ui->menuView->addAction(ui->dockRegisters->toggleViewAction());
    ui->menuView->addAction(ui->dockStack->toggleViewAction());

    qRegisterMetaType<QItemSelection>();
    qRegisterMetaType<uint16_t>("uint16_t");

    QSettings settings;
    restoreGeometry(settings.value(SETTINGS_DEBUGGERWINDOW_GEOMETRY).toByteArray());
    restoreState(settings.value(SETTINGS_DEBUGGERWINDOW_STATE).toByteArray());

    ui->disassemblyView->setModel(disassemblyModel);
    ui->memoryView->setModel(memoryModel);

    ui->memoryView->resizeColumnsToContents();

    connect(ui->actionToggleDebugging, SIGNAL(triggered(bool)), this, SLOT(SlotToggleDebugging(bool)));
    connect(ui->actionStep, SIGNAL(triggered()), this, SLOT(SlotStep()));
    connect(ui->actionRunToLine, SIGNAL(triggered()), this, SLOT(SlotRunToLine()));
    connect(ui->actionDisassemble, SIGNAL(triggered()), this, SLOT(SlotDisassembleAddress()));
    connect(this, SIGNAL(SignalDebuggerWindowClosed()), parent, SLOT(SlotDebuggerWindowClosed()));
    connect(this, SIGNAL(SignalUpdateReady(uint16_t)), this, SLOT(SlotProcessUpdate(uint16_t)));
    connect(this, SIGNAL(SignalReenableActions()), this, SLOT(SlotReenableActions()));
    connect(this, SIGNAL(SignalObjectsChanged()), this, SLOT(SlotObjectsChanged()));
    connect(this, SIGNAL(SignalMemoryChanged(uint16_t, uint16_t)), this, SLOT(SlotMemoryChanged(uint16_t, uint16_t)));
}


DebuggerWindow::~DebuggerWindow()
{
    delete ui;
}


void DebuggerWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue(SETTINGS_DEBUGGERWINDOW_GEOMETRY, saveGeometry());
    settings.setValue(SETTINGS_DEBUGGERWINDOW_STATE, saveState());

    emit SignalDebuggerWindowClosed();

    QWidget::closeEvent(event);
}


void DebuggerWindow::SetEmulatorObjects(Memory *newMemory, Cpu *newCpu, Interrupt *newInterrupt)
{
    //This function runs in the thread context of the Emulator worker thread.

    // All values should be NULL or not NULL. If only some are NULL, treat them all as NULL.
    if (newMemory == NULL || newCpu == NULL || newInterrupt == NULL)
    {
        memory = NULL;
        cpu = NULL;
        interrupt = NULL;
    }
    else
    {
        memory = newMemory;
        cpu = newCpu;
        interrupt = newInterrupt;
    }

    // Run the rest in current thread context.
    emit SignalObjectsChanged();
}


bool DebuggerWindow::ShouldRun(uint16_t pc)
{
    //This function runs in the thread context of the Emulator worker thread.

    // Run if we're in single step mode, or we have a run-to address and we're not there.
    return debuggingEnabled == false || (singleStep == true || (runToAddress != 0xFFFF && pc != runToAddress));
}


void DebuggerWindow::SetCurrentOp(uint16_t pc)
{
    //This function runs in the thread context of the Emulator worker thread.

    // Stop single step mode.
    if (singleStep == true)
    {
        // Re-Enable controls. Emit a signal, since we are running in a non-main thread.
        emit SignalReenableActions();
    }
    singleStep = false;

    // If we've hit the run-to address, reset the variable.
    // 0xFFFF is a data register, so it's safe to use for an invalid address value.
    if (runToAddress == pc)
    {
        runToAddress = 0xFFFF;

        // Re-Enable controls. Emit a signal, since we are running in a non-main thread.
        emit SignalReenableActions();
    }

    emit SignalUpdateReady(pc);
}


void DebuggerWindow::MemoryChanged(uint16_t address, uint16_t len)
{
    //This function runs in the thread context of the Emulator worker thread.

    // Run the rest in current thread context.
    emit SignalMemoryChanged(address, len);
}


void DebuggerWindow::UpdateStack()
{
    if (cpu == NULL || memory == NULL)
        return;

    currentSp = cpu->reg.sp;

    // Let the view determine how many rows to be displayed.
    const int rowCount = ui->stackView->rowCount();

    ui->stackView->clearContents();

    for (int i = 0; i < rowCount; i++)
    {
        uint16_t address = currentSp + (i * 2);
        QTableWidgetItem *item = new QTableWidgetItem(UiUtils::FormatHexWord(address));
        ui->stackView->setItem(i, 0, item);

        uint16_t value = memory->ReadRawByte(address) | (memory->ReadRawByte(address + 1) << 8);
        item = new QTableWidgetItem(UiUtils::FormatHexWord(value));
        ui->stackView->setItem(i, 1, item);
    }
}


void DebuggerWindow::UpdateWidgets(uint16_t pc)
{
    disassemblyModel->AddRow(pc, memory->GetBytePtr(0));

    int rowIndex = disassemblyModel->GetRowIndex(pc);
    if (rowIndex >= 0)
    {
        disassemblyModel->SetCurrentRow(rowIndex);
        ui->disassemblyView->scrollTo(ui->disassemblyView->model()->index(rowIndex, 0));
        ui->disassemblyView->viewport()->update();
    }

    if (cpu != NULL)
    {
        ui->chkFlagZ->setChecked(cpu->reg.flags.z);
        ui->chkFlagN->setChecked(cpu->reg.flags.n);
        ui->chkFlagH->setChecked(cpu->reg.flags.h);
        ui->chkFlagC->setChecked(cpu->reg.flags.c);
        ui->txtAF->setText(UiUtils::FormatHexWord(cpu->reg.af));
        ui->txtBC->setText(UiUtils::FormatHexWord(cpu->reg.bc));
        ui->txtDE->setText(UiUtils::FormatHexWord(cpu->reg.de));
        ui->txtHL->setText(UiUtils::FormatHexWord(cpu->reg.hl));
        ui->txtPC->setText(UiUtils::FormatHexWord(cpu->reg.pc));
        ui->txtSP->setText(UiUtils::FormatHexWord(cpu->reg.sp));

        if (currentSp != cpu->reg.sp)
            UpdateStack();
    }

    if (memory != NULL)
    {
        ui->txtRegP1->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegP1)));
        ui->txtRegIE->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegIE)));
        ui->txtRegIF->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegIF)));
        ui->txtRegDIV->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegDIV)));
        ui->txtRegTIMA->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegTIMA)));
        ui->txtRegTMA->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegTMA)));
        ui->txtRegTAC->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegTAC)));
        ui->txtRegLCDC->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegLCDC)));
        ui->txtRegSTAT->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegSTAT)));
        ui->txtRegSCX->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegSCX)));
        ui->txtRegSCY->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegSCY)));
        ui->txtRegWX->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegWX)));
        ui->txtRegWY->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegWY)));
        ui->txtRegLY->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegLY)));
        ui->txtRegLYC->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegLYC)));
        ui->txtRegDMA->setText(UiUtils::FormatHexByte(memory->ReadRawByte(eRegDMA)));
        ui->txtRegCurDMA->setText(UiUtils::FormatHexByte(OAM_RAM_START + memory->GetDmaOffset()));
    }

    if (interrupt != NULL)
        ui->chkRegIME->setChecked(interrupt->Enabled());
}


void DebuggerWindow::SlotProcessUpdate(uint16_t pc)
{
    UpdateWidgets(pc);

    // Add current instruction to call stack.
    Opcode opcode = Opcode::GetOpcode(pc, memory->GetBytePtr(pc));
    new QListWidgetItem(opcode.ToString(), ui->callStackView);
}


void DebuggerWindow::SlotToggleDebugging(bool checked)
{
    debuggingEnabled = checked;

    if (debuggingEnabled)
    {
        ui->actionToggleDebugging->setText("Stop Debugging");
        ui->actionStep->setEnabled(true);
        ui->actionRunToLine->setEnabled(true);
        ui->actionDisassemble->setEnabled(true);

        // Update memory table with new values.
        memoryModel->SetMemory(memory);

        // Update widgets with new values.
        if (cpu != NULL)
        {
            // Sleep to make sure the emulator worker thread has stopped running.
            // I don't feel like adding proper thread synchronization right now.
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            UpdateWidgets(cpu->reg.pc);
        }

        // Clear the call stack. It would be too slow to log the call stack when not debugging.
        ui->callStackView->clear();
    }
    else
    {
        ui->actionToggleDebugging->setText("Start Debugging");
        ui->actionStep->setEnabled(false);
        ui->actionRunToLine->setEnabled(false);
        ui->actionDisassemble->setEnabled(false);
    }
}


void DebuggerWindow::SlotStep()
{
    singleStep = true;

    // Disable controls while running.
    ui->actionRunToLine->setEnabled(false);
    ui->actionStep->setEnabled(false);
    ui->actionDisassemble->setEnabled(false);
}


void DebuggerWindow::SlotRunToLine()
{
    QItemSelectionModel *selection = ui->disassemblyView->selectionModel();

    if (!selection->hasSelection())
    {
        UiUtils::MessageBox("No row selected");
        return;
    }

    runToAddress = disassemblyModel->GetAddressOfRow(selection->selectedRows()[0].row());

    // Disable controls while running.
    ui->actionRunToLine->setEnabled(false);
    ui->actionStep->setEnabled(false);
    ui->actionDisassemble->setEnabled(false);

    // Clear selection so we can see when the line has been reached. TODO: Add "wait" dialog while running.
    ui->disassemblyView->clearSelection();
}


void DebuggerWindow::SlotDisassembleAddress()
{
    if (memory == NULL)
    {
        UiUtils::MessageBox("Game not loaded");
        return;
    }

    AddressDialog dialog(this);
    dialog.setModal(true);
    int ret = dialog.exec();

    if (ret == 1)
    {
        disassemblyModel->AddRow(dialog.address, memory->GetBytePtr(0));
    }
}


void DebuggerWindow::SlotReenableActions()
{
    ui->actionRunToLine->setEnabled(true);
    ui->actionStep->setEnabled(true);
    ui->actionDisassemble->setEnabled(true);
}


void DebuggerWindow::SlotObjectsChanged()
{
    memoryModel->SetMemory(memory);

    if (cpu != NULL)
        UpdateWidgets(cpu->reg.pc);
}


void DebuggerWindow::SlotMemoryChanged(uint16_t address, uint16_t len)
{
    // Memory has changed, so the disassembled opcodes are no longer valid.
    disassemblyModel->RemoveRows(address, len);

    // Only update memory table when debugging to avoid slowing things down.
    if (debuggingEnabled == true)
    {
        memoryModel->InvalidateMemory(address, len);
    }
}