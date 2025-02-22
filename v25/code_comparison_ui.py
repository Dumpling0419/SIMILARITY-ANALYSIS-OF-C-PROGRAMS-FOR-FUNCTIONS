import sys
import os
import subprocess
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                            QHBoxLayout, QPushButton, QLabel, QFileDialog, 
                            QListWidget, QMessageBox, QProgressBar, QFrame,
                            QStyle, QStyleFactory, QSplitter)
from PyQt6.QtCore import Qt, QThread, pyqtSignal
from PyQt6.QtGui import QFont, QPalette, QColor
from results_window import ResultsWindow
from score_config_editor import ScoreConfigEditor

class ComparisonWorker(QThread):
    progress = pyqtSignal(int)
    finished = pyqtSignal(list)
    error = pyqtSignal(str)

    def __init__(self, instructor_file, student_files):
        super().__init__()
        self.instructor_file = instructor_file
        self.student_files = student_files

    def run(self):
        try:
            results = []
            total_files = len(self.student_files)
            
            for i, student_file in enumerate(self.student_files):
                result = self.compare_files(self.instructor_file, student_file)
                results.append((student_file, result))
                progress = int((i + 1) / total_files * 100)
                self.progress.emit(progress)
            
            self.finished.emit(results)
        
        except Exception as e:
            self.error.emit(str(e))

    def compare_files(self, instructor_file, student_file):
        try:
            result = subprocess.run(['./parser', instructor_file, student_file], 
                                 capture_output=True, text=True)
            return result.stdout
        except Exception as e:
            return f"Error comparing files: {str(e)}"

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.instructor_file = None
        self.student_files = []
        self.init_ui()
        self.setup_styling()

    def setup_styling(self):
        # Ana pencere stili
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f0f0f0;
            }
            QPushButton {
                background-color: #2c3e50;
                color: white;
                border: none;
                padding: 8px 15px;
                border-radius: 4px;
                font-weight: bold;
                min-width: 120px;
            }
            QPushButton:hover {
                background-color: #34495e;
            }
            QPushButton:pressed {
                background-color: #2980b9;
            }
            QListWidget {
                background-color: white;
                border: 1px solid #bdc3c7;
                border-radius: 4px;
                padding: 5px;
            }
            QProgressBar {
                border: 1px solid #bdc3c7;
                border-radius: 4px;
                text-align: center;
            }
            QProgressBar::chunk {
                background-color: #3498db;
            }
            QLabel {
                color: #2c3e50;
                font-weight: bold;
            }
            QFrame {
                background-color: white;
                border-radius: 8px;
                padding: 15px;
            }
        """)

    def init_ui(self):
        self.setWindowTitle('Code Comparison Tool')
        self.setGeometry(100, 100, 1000, 700)

        # Ana widget ve layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QVBoxLayout(main_widget)
        main_layout.setSpacing(20)
        main_layout.setContentsMargins(20, 20, 20, 20)

        # Başlık
        title_label = QLabel('Code Comparison and Analysis Tool')
        title_label.setFont(QFont('Arial', 16, QFont.Weight.Bold))
        title_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        main_layout.addWidget(title_label)

        # Score Config Editor butonu
        config_btn = QPushButton('Edit Scoring Configuration')
        config_btn.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_FileDialogDetailedView))
        config_btn.clicked.connect(self.open_score_config_editor)
        main_layout.addWidget(config_btn, alignment=Qt.AlignmentFlag.AlignRight)

        # Instructor bölümü
        instructor_frame = QFrame()
        instructor_layout = QVBoxLayout(instructor_frame)
        
        instructor_header = QLabel('Reference Code')
        instructor_header.setFont(QFont('Arial', 12, QFont.Weight.Bold))
        instructor_layout.addWidget(instructor_header)

        instructor_content = QHBoxLayout()
        self.instructor_label = QLabel('No reference code selected')
        instructor_btn = QPushButton('Select Reference File')
        instructor_btn.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_FileIcon))
        instructor_content.addWidget(self.instructor_label)
        instructor_content.addWidget(instructor_btn)
        instructor_layout.addLayout(instructor_content)
        
        main_layout.addWidget(instructor_frame)

        # Student bölümü
        student_frame = QFrame()
        student_layout = QVBoxLayout(student_frame)
        
        student_header = QLabel('Student Submissions')
        student_header.setFont(QFont('Arial', 12, QFont.Weight.Bold))
        student_layout.addWidget(student_header)

        student_content = QHBoxLayout()
        self.student_list = QListWidget()
        self.student_list.setMinimumHeight(300)
        self.student_list.setSelectionMode(QListWidget.SelectionMode.ExtendedSelection)
        
        button_layout = QVBoxLayout()
        add_student_btn = QPushButton('Add Files')
        add_student_btn.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_FileDialogStart))
        remove_selected_btn = QPushButton('Remove Selected')
        remove_selected_btn.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_TrashIcon))
        clear_student_btn = QPushButton('Clear All')
        clear_student_btn.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_BrowserStop))
        
        button_layout.addWidget(add_student_btn)
        button_layout.addWidget(remove_selected_btn)
        button_layout.addWidget(clear_student_btn)
        button_layout.addStretch()
        
        student_content.addWidget(self.student_list)
        student_content.addLayout(button_layout)
        student_layout.addLayout(student_content)
        
        main_layout.addWidget(student_frame)

        # Alt bölüm
        bottom_frame = QFrame()
        bottom_layout = QVBoxLayout(bottom_frame)
        
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        self.progress_bar.setMinimumWidth(400)
        
        compare_btn = QPushButton('Start Analysis')
        compare_btn.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_CommandLink))
        compare_btn.setMinimumWidth(200)
        
        bottom_layout.addWidget(self.progress_bar)
        bottom_layout.addWidget(compare_btn, alignment=Qt.AlignmentFlag.AlignCenter)
        
        main_layout.addWidget(bottom_frame)

        # Bağlantılar
        instructor_btn.clicked.connect(self.select_instructor_file)
        add_student_btn.clicked.connect(self.select_student_files)
        remove_selected_btn.clicked.connect(self.remove_selected_files)
        clear_student_btn.clicked.connect(self.clear_student_files)
        compare_btn.clicked.connect(self.start_comparison)

        # Durum çubuğu
        self.statusBar().setStyleSheet("background-color: #2c3e50; color: white;")
        self.statusBar().showMessage('Ready')

    def select_instructor_file(self):
        file_name, _ = QFileDialog.getOpenFileName(
            self,
            "Select Reference File",
            "",
            "C Files (*.c);;All Files (*)"
        )
        if file_name:
            self.instructor_file = file_name
            self.instructor_label.setText(os.path.basename(file_name))
            self.statusBar().showMessage(f'Reference file selected: {os.path.basename(file_name)}')

    def select_student_files(self):
        files, _ = QFileDialog.getOpenFileNames(
            self,
            "Select Student Files",
            "",
            "C Files (*.c);;All Files (*)"
        )
        if files:
            self.student_files.extend(files)
            self.update_student_list()
            self.statusBar().showMessage(f'Added {len(files)} student submission(s)')

    def clear_student_files(self):
        self.student_files.clear()
        self.update_student_list()
        self.statusBar().showMessage('Student submission list cleared')

    def update_student_list(self):
        self.student_list.clear()
        for file in self.student_files:
            self.student_list.addItem(os.path.basename(file))

    def start_comparison(self):
        if not self.instructor_file:
            QMessageBox.warning(self, "Warning", "Please select a reference file first!")
            return
        
        if not self.student_files:
            QMessageBox.warning(self, "Warning", "Please add at least one student submission!")
            return

        self.progress_bar.setVisible(True)
        self.statusBar().showMessage('Analysis in progress...')
        self.worker = ComparisonWorker(self.instructor_file, self.student_files)
        self.worker.progress.connect(self.update_progress)
        self.worker.finished.connect(self.comparison_finished)
        self.worker.error.connect(self.comparison_error)
        self.worker.start()

    def update_progress(self, value):
        self.progress_bar.setValue(value)
        self.statusBar().showMessage(f'Processing... {value}%')

    def comparison_finished(self, results):
        self.progress_bar.setVisible(False)
        self.statusBar().showMessage('Analysis completed')
        results_window = ResultsWindow(results, self)
        results_window.exec()

    def comparison_error(self, error_msg):
        self.progress_bar.setVisible(False)
        self.statusBar().showMessage('Error occurred during analysis')
        QMessageBox.critical(self, "Error", f"An error occurred: {error_msg}")

    def remove_selected_files(self):
        selected_items = self.student_list.selectedItems()
        if not selected_items:
            return
            
        for item in selected_items:
            file_name = item.text()
            # Listeden ve student_files listesinden kaldır
            self.student_files = [f for f in self.student_files if os.path.basename(f) != file_name]
            self.student_list.takeItem(self.student_list.row(item))
            
        self.statusBar().showMessage(f'Removed {len(selected_items)} selected submission(s)')

    def open_score_config_editor(self):
        editor = ScoreConfigEditor(self)
        editor.exec()

def main():
    app = QApplication(sys.argv)
    app.setStyle(QStyleFactory.create('Fusion'))
    window = MainWindow()
    window.show()
    sys.exit(app.exec())

if __name__ == '__main__':
    main() 