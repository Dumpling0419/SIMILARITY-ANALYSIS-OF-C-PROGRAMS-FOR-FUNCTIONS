from PyQt6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QTableWidget, 
                            QTableWidgetItem, QPushButton, QLabel, QHeaderView,
                            QFrame, QFileDialog, QMessageBox, QStyle)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont, QColor
import os

class ResultsWindow(QDialog):
    def __init__(self, results, parent=None):
        super().__init__(parent)
        self.results = results
        self.init_ui()
        self.setup_styling()

    def setup_styling(self):
        self.setStyleSheet("""
            QDialog {
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
            QTableWidget {
                background-color: white;
                border: 1px solid #bdc3c7;
                border-radius: 4px;
                gridline-color: #ecf0f1;
            }
            QTableWidget::item {
                padding: 5px;
            }
            QTableWidget::item:selected {
                background-color: #3498db;
                color: white;
            }
            QHeaderView::section {
                background-color: #2c3e50;
                color: white;
                padding: 8px;
                border: none;
                font-weight: bold;
            }
            QFrame {
                background-color: white;
                border-radius: 8px;
                padding: 15px;
            }
        """)

    def init_ui(self):
        self.setWindowTitle('Analysis Results')
        self.setGeometry(200, 200, 1200, 800)
        
        layout = QVBoxLayout(self)
        layout.setSpacing(20)
        layout.setContentsMargins(20, 20, 20, 20)

        # Başlık
        title_label = QLabel('Code Analysis Results')
        title_label.setFont(QFont('Arial', 16, QFont.Weight.Bold))
        title_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(title_label)

        # Tablo frame
        table_frame = QFrame()
        table_layout = QVBoxLayout(table_frame)
        
        # Tablo oluştur
        self.table = QTableWidget()
        self.table.setColumnCount(4)
        self.table.setHorizontalHeaderLabels(['Student Submission', 'Raw Score', 'Normalized Score', 'Detailed Analysis'])
        
        # Maksimum skor (75 üzerinden)
        MAX_POSSIBLE_SCORE = 75

        # Sonuçları tabloya ekle
        self.table.setRowCount(len(self.results))
        for i, (file, result) in enumerate(self.results):
            # Dosya adı
            file_item = QTableWidgetItem(os.path.basename(file))
            self.table.setItem(i, 0, file_item)
            
            # Raw Skor
            try:
                raw_score = self.extract_score(result)
                raw_score_item = QTableWidgetItem(str(raw_score))
                raw_score_item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
                self.table.setItem(i, 1, raw_score_item)
                
                # Normalize edilmiş skor (75 üzerinden 100'e çevir)
                if isinstance(raw_score, int):
                    normalized_score = round((raw_score / MAX_POSSIBLE_SCORE) * 100)
                    normalized_score_item = QTableWidgetItem(f"{normalized_score}%")
                    normalized_score_item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
                    
                    # Normalize edilmiş skora göre renklendirme
                    if normalized_score >= 80:
                        normalized_score_item.setBackground(QColor("#27ae60"))  # Yeşil
                    elif normalized_score >= 60:
                        normalized_score_item.setBackground(QColor("#f1c40f"))  # Sarı
                    else:
                        normalized_score_item.setBackground(QColor("#e74c3c"))  # Kırmızı
                    normalized_score_item.setForeground(QColor("white"))
                    
                    self.table.setItem(i, 2, normalized_score_item)
                else:
                    self.table.setItem(i, 2, QTableWidgetItem("N/A"))
            except:
                self.table.setItem(i, 1, QTableWidgetItem("N/A"))
                self.table.setItem(i, 2, QTableWidgetItem("N/A"))
            
            # Detaylar
            details = self.extract_comparison_details(result)
            details_item = QTableWidgetItem(details)
            self.table.setItem(i, 3, details_item)

        # Tablo sütunlarını ayarla
        header = self.table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(3, QHeaderView.ResizeMode.Stretch)

        table_layout.addWidget(self.table)
        layout.addWidget(table_frame)

        # Butonlar
        button_frame = QFrame()
        button_layout = QHBoxLayout(button_frame)
        button_layout.setSpacing(10)
        
        export_btn = QPushButton('Export Results')
        export_btn.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_DialogSaveButton))
        close_btn = QPushButton('Close')
        close_btn.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_DialogCloseButton))
        
        export_btn.clicked.connect(self.export_results)
        close_btn.clicked.connect(self.close)
        
        button_layout.addStretch()
        button_layout.addWidget(export_btn)
        button_layout.addWidget(close_btn)
        button_layout.addStretch()
        
        layout.addWidget(button_frame)

    def extract_score(self, result):
        try:
            score_line = [line for line in result.split('\n') if 'Total Score:' in line][0]
            return int(score_line.split(':')[1].strip())
        except:
            return "N/A"

    def export_results(self):
        # Sonuçları CSV olarak dışa aktar
        try:
            from datetime import datetime
            import csv
            
            filename = f"comparison_results_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
            
            with open(filename, 'w', newline='') as file:
                writer = csv.writer(file)
                writer.writerow(['Student File', 'Raw Score (out of 75)', 'Normalized Score (%)', 'Detailed Analysis'])
                
                for i in range(self.table.rowCount()):
                    file_name = self.table.item(i, 0).text()
                    raw_score = self.table.item(i, 1).text()
                    normalized_score = self.table.item(i, 2).text()
                    details = self.table.item(i, 3).text()
                    writer.writerow([file_name, raw_score, normalized_score, details])
            
            from PyQt6.QtWidgets import QMessageBox
            QMessageBox.information(self, "Success", f"Results exported to {filename}")
            
        except Exception as e:
            from PyQt6.QtWidgets import QMessageBox
            QMessageBox.critical(self, "Error", f"Failed to export results: {str(e)}")

    def extract_comparison_details(self, details):
        try:
            lines = details.split('\n')
            formatted_details = []
            current_section = None
            
            # Karşılaştırma başlığı için ayraç ekle
            formatted_details.append("\n" + "="*50 + "\nCOMPARISON RESULTS\n" + "="*50 + "\n")
            
            for line in lines:
                # AST ile ilgili satırları atla
                if "AST" in line:
                    continue
                    
                # Yeni bir tablo başlangıcını kontrol et
                if "Variable Table for" in line:
                    if current_section != "Variable Table":
                        formatted_details.append("\n" + "-"*40 + "\n")
                    current_section = "Variable Table"
                    formatted_details.append("\n" + line)
                elif "Printing Variable Table:" in line:
                    formatted_details.append(line)
                elif "Printing Match Table:" in line:
                    formatted_details.append("\n" + "-"*40 + "\n")
                    current_section = "Match Table"
                    formatted_details.append(line)
                elif "Printing Function Call Table:" in line:
                    formatted_details.append("\n" + "-"*40 + "\n")
                    current_section = "Function Call Table"
                    formatted_details.append(line)
                elif "Printing Function Call Match Table:" in line:
                    formatted_details.append("\n" + "-"*40 + "\n")
                    current_section = "Function Call Match Table"
                    formatted_details.append(line)
                elif "Total Score:" in line:
                    formatted_details.append("\n" + "-"*40 + "\n")
                    formatted_details.append(line)
                    formatted_details.append("\n" + "="*50 + "\n")  # Karşılaştırma sonu için ayraç
                # Eğer satır bir tabloya aitse ve AST içermiyorsa ekle
                elif current_section and line.strip() and "AST" not in line:
                    formatted_details.append(line)
            
            return '\n'.join(formatted_details)
        except Exception as e:
            return f"Error processing details: {str(e)}" 