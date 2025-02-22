from PyQt6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QTabWidget,
                            QWidget, QPushButton, QLabel, QLineEdit, QSpinBox,
                            QFormLayout, QScrollArea, QMessageBox, QFrame,
                            QComboBox, QCheckBox)
from PyQt6.QtCore import Qt
import json
import os

class ScoreConfigEditor(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.config_file = "score_config.json"
        self.current_config = self.load_config()
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
            QLineEdit, QSpinBox, QComboBox {
                padding: 5px;
                border: 1px solid #bdc3c7;
                border-radius: 4px;
            }
            QFrame {
                background-color: white;
                border-radius: 8px;
                padding: 15px;
            }
            QPushButton.delete-btn {
                background-color: #e74c3c;
                min-width: 30px;
                padding: 5px;
            }
            QPushButton.delete-btn:hover {
                background-color: #c0392b;
            }
        """)

    def create_delete_button(self, parent_layout, frame):
        delete_btn = QPushButton("×")
        delete_btn.setFixedSize(30, 30)
        delete_btn.setStyleSheet("""
            QPushButton {
                background-color: #e74c3c;
                color: white;
                border-radius: 15px;
                font-weight: bold;
                font-size: 16px;
            }
            QPushButton:hover {
                background-color: #c0392b;
            }
        """)
        delete_btn.clicked.connect(lambda: self.delete_frame(parent_layout, frame))
        return delete_btn

    def delete_frame(self, layout, frame):
        layout.removeRow(frame)
        frame.deleteLater()

    def add_new_variable(self, form_layout):
        frame = QFrame()
        frame_layout = QHBoxLayout(frame)
        
        form = QFormLayout()
        func_edit = QLineEdit()
        var_edit = QLineEdit()
        points_spin = QSpinBox()
        points_spin.setRange(0, 100)
        
        form.addRow("Function:", func_edit)
        form.addRow("Variable:", var_edit)
        form.addRow("Points:", points_spin)
        
        frame_layout.addLayout(form)
        frame_layout.addWidget(self.create_delete_button(form_layout, frame))
        
        form_layout.addRow(frame)

    def add_new_function(self, form_layout):
        frame = QFrame()
        frame_layout = QHBoxLayout(frame)
        
        form = QFormLayout()
        name_edit = QLineEdit()
        points_spin = QSpinBox()
        points_spin.setRange(0, 100)

        # Parameters input
        params_edit = QLineEdit()
        params_edit.setPlaceholderText("Enter parameters separated by comma")
        
        # Return variable input
        return_var_edit = QLineEdit()
        return_var_edit.setPlaceholderText("Enter function name for return expression")
        
        form.addRow("Function Name:", name_edit)
        form.addRow("Points:", points_spin)
        form.addRow("Parameters:", params_edit)
        form.addRow("Return Variable:", return_var_edit)
        
        frame_layout.addLayout(form)
        frame_layout.addWidget(self.create_delete_button(form_layout, frame))
        
        form_layout.addRow(frame)

    def add_new_function_call(self, form_layout):
        frame = QFrame()
        frame_layout = QHBoxLayout(frame)
        
        form = QFormLayout()
        desc_edit = QLineEdit()
        loc_edit = QLineEdit()
        func_name_edit = QLineEdit()
        points_spin = QSpinBox()
        points_spin.setRange(0, 100)
        
        # Parameters section
        param_count_spin = QSpinBox()
        param_count_spin.setRange(0, 10)
        param_count_spin.valueChanged.connect(lambda x: self.update_param_fields(x, param_container))
        
        param_container = QFrame()
        param_layout = QVBoxLayout(param_container)
        
        form.addRow("Description:", desc_edit)
        form.addRow("Location:", loc_edit)
        form.addRow("Function Name:", func_name_edit)
        form.addRow("Points:", points_spin)
        form.addRow("Parameter Count:", param_count_spin)
        form.addRow("Parameters:", param_container)
        
        frame_layout.addLayout(form)
        frame_layout.addWidget(self.create_delete_button(form_layout, frame))
        
        form_layout.addRow(frame)

    def update_param_fields(self, count, container):
        # Clear existing fields
        for i in reversed(range(container.layout().count())): 
            container.layout().itemAt(i).widget().setParent(None)
        
        # Add new parameter fields
        for i in range(count):
            param_frame = QFrame()
            param_layout = QHBoxLayout(param_frame)
            
            param_type = QComboBox()
            param_type.addItems(["constant", "variable_expression"])
            
            value_edit = QLineEdit()
            value_edit.setPlaceholderText("Value/Expression")
            
            param_layout.addWidget(QLabel(f"Param {i+1}:"))
            param_layout.addWidget(param_type)
            param_layout.addWidget(value_edit)
            
            container.layout().addWidget(param_frame)

    def collect_form_data(self):
        config = {"variables": [], "functions": [], "function_calls": []}
        
        # Variables topla
        variables_form = self.findChild(QTabWidget).widget(0).findChild(QScrollArea).widget().layout()
        for i in range(variables_form.rowCount()):
            frame = variables_form.itemAt(i).widget()
            if frame and isinstance(frame, QFrame):
                layout = frame.layout()
                form = layout.itemAt(0).layout()
                
                func_edit = form.itemAt(0, QFormLayout.ItemRole.FieldRole).widget()
                var_edit = form.itemAt(1, QFormLayout.ItemRole.FieldRole).widget()
                points_spin = form.itemAt(2, QFormLayout.ItemRole.FieldRole).widget()
                
                if func_edit.text() and var_edit.text():
                    config["variables"].append({
                        "teacher_function": func_edit.text(),
                        "teacher_var": var_edit.text(),
                        "points": points_spin.value()
                    })
        
        # Functions topla
        functions_form = self.findChild(QTabWidget).widget(1).findChild(QScrollArea).widget().layout()
        for i in range(functions_form.rowCount()):
            frame = functions_form.itemAt(i).widget()
            if frame and isinstance(frame, QFrame):
                layout = frame.layout()
                form = layout.itemAt(0).layout()
                
                name_edit = form.itemAt(0, QFormLayout.ItemRole.FieldRole).widget()
                points_spin = form.itemAt(1, QFormLayout.ItemRole.FieldRole).widget()
                params_edit = form.itemAt(2, QFormLayout.ItemRole.FieldRole).widget()
                return_var_edit = form.itemAt(3, QFormLayout.ItemRole.FieldRole).widget()
                
                if name_edit.text():
                    func_data = {
                        "teacher_name": name_edit.text(),
                        "points": points_spin.value(),
                        "parameters": [p.strip() for p in params_edit.text().split(",") if p.strip()],
                        "return_var": f"Return Expression in {return_var_edit.text() or name_edit.text()}"
                    }
                    config["functions"].append(func_data)
        
        # Function Calls topla
        calls_form = self.findChild(QTabWidget).widget(2).findChild(QScrollArea).widget().layout()
        for i in range(calls_form.rowCount()):
            frame = calls_form.itemAt(i).widget()
            if frame and isinstance(frame, QFrame):
                layout = frame.layout()
                form = layout.itemAt(0).layout()
                
                desc_edit = form.itemAt(0, QFormLayout.ItemRole.FieldRole).widget()
                loc_edit = form.itemAt(1, QFormLayout.ItemRole.FieldRole).widget()
                func_name_edit = form.itemAt(2, QFormLayout.ItemRole.FieldRole).widget()
                points_spin = form.itemAt(3, QFormLayout.ItemRole.FieldRole).widget()
                param_container = form.itemAt(5, QFormLayout.ItemRole.FieldRole).widget()
                
                if desc_edit.text() and loc_edit.text():
                    params = []
                    for j in range(param_container.layout().count()):
                        param_frame = param_container.layout().itemAt(j).widget()
                        param_type = param_frame.layout().itemAt(1).widget()
                        value_edit = param_frame.layout().itemAt(2).widget()
                        
                        param_data = {
                            "type": param_type.currentText()
                        }
                        
                        if param_type.currentText() == "constant":
                            try:
                                param_data["value"] = int(value_edit.text())
                            except ValueError:
                                param_data["value"] = 0
                        else:
                            param_data["expr"] = value_edit.text()
                        
                        params.append(param_data)
                    
                    call_data = {
                        "description": desc_edit.text(),
                        "location": loc_edit.text(),
                        "points": points_spin.value(),
                        "teacher_call": {
                            "function_name": func_name_edit.text(),
                            "param_count": len(params),
                            "params": params
                        }
                    }
                    config["function_calls"].append(call_data)
        
        return config

    def save_config(self):
        temp_file = self.config_file + ".tmp"
        try:
            config = self.collect_form_data()
            
            with open(temp_file, 'w', encoding='utf-8') as f:
                json.dump(config, f, indent=4, ensure_ascii=False)
            
            if os.path.exists(self.config_file):
                os.remove(self.config_file)
            os.rename(temp_file, self.config_file)
            
            # Update current config
            self.current_config = config
            
            QMessageBox.information(self, "Success", "Configuration saved successfully!")
            
            super().accept()
            
        except PermissionError:
            QMessageBox.critical(self, "Error", "Permission denied. Cannot write to the file.")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to save configuration: {str(e)}")
            if temp_file and os.path.exists(temp_file):
                os.remove(temp_file)

    def load_config(self):
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
            return {"variables": [], "functions": [], "function_calls": []}
        except json.JSONDecodeError:
            QMessageBox.warning(self, "Warning", "Invalid JSON format in configuration file. Starting with empty configuration.")
            return {"variables": [], "functions": [], "function_calls": []}
        except Exception as e:
            QMessageBox.warning(self, "Warning", f"Error loading configuration: {str(e)}")
            return {"variables": [], "functions": [], "function_calls": []} 

    def init_ui(self):
        self.setWindowTitle('Code Comparison Tool')
        self.setGeometry(200, 200, 800, 600)

        layout = QVBoxLayout(self)
        
        tab_widget = QTabWidget()
        
        variables_tab = QWidget()
        variables_layout = QVBoxLayout(variables_tab)
        
        variables_scroll = QScrollArea()
        variables_scroll.setWidgetResizable(True)
        variables_content = QWidget()
        variables_form = QFormLayout(variables_content)
        
        for var in self.current_config.get("variables", []):
            frame = QFrame()
            frame_layout = QHBoxLayout(frame)
            
            form = QFormLayout()
            func_edit = QLineEdit(var.get("teacher_function", ""))
            var_edit = QLineEdit(var.get("teacher_var", ""))
            points_spin = QSpinBox()
            points_spin.setRange(0, 100)
            points_spin.setValue(var.get("points", 0))
            
            form.addRow("Function:", func_edit)
            form.addRow("Variable:", var_edit)
            form.addRow("Points:", points_spin)
            
            frame_layout.addLayout(form)
            frame_layout.addWidget(self.create_delete_button(variables_form, frame))
            
            variables_form.addRow(frame)
        
        add_var_btn = QPushButton("Add New Variable")
        add_var_btn.clicked.connect(lambda: self.add_new_variable(variables_form))
        
        variables_scroll.setWidget(variables_content)
        variables_layout.addWidget(variables_scroll)
        variables_layout.addWidget(add_var_btn)
        
        functions_tab = QWidget()
        functions_layout = QVBoxLayout(functions_tab)
        
        functions_scroll = QScrollArea()
        functions_scroll.setWidgetResizable(True)
        functions_content = QWidget()
        functions_form = QFormLayout(functions_content)
        
        for func in self.current_config.get("functions", []):
            frame = QFrame()
            frame_layout = QHBoxLayout(frame)
            
            form = QFormLayout()
            name_edit = QLineEdit(func.get("teacher_name", ""))
            points_spin = QSpinBox()
            points_spin.setRange(0, 100)
            points_spin.setValue(func.get("points", 0))
            
            params_edit = QLineEdit()
            params_edit.setText(", ".join(func.get("parameters", [])))
            params_edit.setPlaceholderText("Enter parameters separated by comma")
            
            return_var_edit = QLineEdit()
            return_var = func.get("return_var", "")
            if return_var.startswith("Return Expression in "):
                return_var = return_var[len("Return Expression in "):]
            return_var_edit.setText(return_var)
            return_var_edit.setPlaceholderText("Enter function name for return expression")
            
            form.addRow("Function Name:", name_edit)
            form.addRow("Points:", points_spin)
            form.addRow("Parameters:", params_edit)
            form.addRow("Return Variable:", return_var_edit)
            
            frame_layout.addLayout(form)
            frame_layout.addWidget(self.create_delete_button(functions_form, frame))
            
            functions_form.addRow(frame)
        
        add_func_btn = QPushButton("Add New Function")
        add_func_btn.clicked.connect(lambda: self.add_new_function(functions_form))
        
        functions_scroll.setWidget(functions_content)
        functions_layout.addWidget(functions_scroll)
        functions_layout.addWidget(add_func_btn)
        
        # Function Calls tab
        calls_tab = QWidget()
        calls_layout = QVBoxLayout(calls_tab)
        
        calls_scroll = QScrollArea()
        calls_scroll.setWidgetResizable(True)
        calls_content = QWidget()
        calls_form = QFormLayout(calls_content)
        
        # Mevcut function calls ekle
        for call in self.current_config.get("function_calls", []):
            frame = QFrame()
            frame_layout = QHBoxLayout(frame)
            
            form = QFormLayout()
            desc_edit = QLineEdit(call.get("description", ""))
            loc_edit = QLineEdit(call.get("location", ""))
            func_name_edit = QLineEdit(call.get("teacher_call", {}).get("function_name", ""))
            points_spin = QSpinBox()
            points_spin.setRange(0, 100)
            points_spin.setValue(call.get("points", 0))
            
            # Parameters section
            param_count_spin = QSpinBox()
            param_count_spin.setRange(0, 10)
            param_count = call.get("teacher_call", {}).get("param_count", 0)
            param_count_spin.setValue(param_count)
            
            param_container = QFrame()
            param_layout = QVBoxLayout(param_container)
            
            # Add existing parameters
            params = call.get("teacher_call", {}).get("params", [])
            for i, param in enumerate(params):
                param_frame = QFrame()
                param_layout = QHBoxLayout(param_frame)
                
                param_type = QComboBox()
                param_type.addItems(["constant", "variable_expression"])
                param_type.setCurrentText(param.get("type", "constant"))
                
                value_edit = QLineEdit()
                if param.get("type") == "constant":
                    value_edit.setText(str(param.get("value", "")))
                else:
                    value_edit.setText(param.get("expr", ""))
                value_edit.setPlaceholderText("Value/Expression")
                
                param_layout.addWidget(QLabel(f"Param {i+1}:"))
                param_layout.addWidget(param_type)
                param_layout.addWidget(value_edit)
                
                param_container.layout().addWidget(param_frame)
            
            form.addRow("Description:", desc_edit)
            form.addRow("Location:", loc_edit)
            form.addRow("Function Name:", func_name_edit)
            form.addRow("Points:", points_spin)
            form.addRow("Parameter Count:", param_count_spin)
            form.addRow("Parameters:", param_container)
            
            param_count_spin.valueChanged.connect(lambda x: self.update_param_fields(x, param_container))
            
            frame_layout.addLayout(form)
            frame_layout.addWidget(self.create_delete_button(calls_form, frame))
            
            calls_form.addRow(frame)
        
        # Yeni function call ekleme butonu
        add_call_btn = QPushButton("Add New Function Call")
        add_call_btn.clicked.connect(lambda: self.add_new_function_call(calls_form))
        
        calls_scroll.setWidget(calls_content)
        calls_layout.addWidget(calls_scroll)
        calls_layout.addWidget(add_call_btn)
        
        # Tab'ları ekle
        tab_widget.addTab(variables_tab, "Variables")
        tab_widget.addTab(functions_tab, "Functions")
        tab_widget.addTab(calls_tab, "Function Calls")
        
        layout.addWidget(tab_widget)
        
        # Kaydet ve İptal butonları
        button_layout = QHBoxLayout()
        save_btn = QPushButton("Save Configuration")
        cancel_btn = QPushButton("Cancel")
        
        save_btn.clicked.connect(self.save_config)
        cancel_btn.clicked.connect(self.reject)
        
        button_layout.addStretch()
        button_layout.addWidget(save_btn)
        button_layout.addWidget(cancel_btn)
        
        layout.addLayout(button_layout) 
        
