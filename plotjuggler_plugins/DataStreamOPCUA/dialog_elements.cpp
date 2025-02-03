#include "dialog_elements.h"

// Number element
FormNumberElement::FormNumberElement(const QString& title, int initialValue) : FormElement<int, QSpinBox>(title)
{
  this->edit_ = new QSpinBox;
  this->edit_->setValue(initialValue);
  this->edit_->setMinimum(1);
  this->edit_->setMaximum(std::numeric_limits<int>::max());
  this->edit_->setFixedHeight(this->formRowHeight_);
}

int FormNumberElement::getValue()
{
  return this->edit_->value();
}

void FormNumberElement::clear()
{
  this->edit_->setValue(this->edit_->minimum());
}

void FormNumberElement::onChange(const QObject* receiver, const char* slot)
{
  QObject::connect(this->edit_, SIGNAL(valueChanged(int)), receiver, slot);
}

// String element
FormStringElement::FormStringElement(const QString& title, const QString& placeholder)
  : FormElement<QString, QLineEdit>(title)
{
  this->edit_->setFixedHeight(this->formRowHeight_);
  this->edit_->setPlaceholderText(placeholder);
}
QString FormStringElement::getValue()
{
  return this->edit_->text();
}
void FormStringElement::clear()
{
  this->edit_->clear();
}
void FormStringElement::onChange(const QObject* receiver, const char* slot)
{
  QObject::connect(this->edit_, SIGNAL(textChanged(QString)), receiver, slot);
}

// Button element
FormButtonElement::FormButtonElement(const QString& title) : FormElement<void, QPushButton>(title)
{
  this->edit_->setText(title);
  this->edit_->setMinimumHeight(this->formRowHeight_);
  this->role_ = QDialogButtonBox::ButtonRole::NoRole;
}

FormButtonElement::FormButtonElement(const QString& title, QDialogButtonBox::ButtonRole role)
  : FormElement<void, QPushButton>(title)
{
  this->edit_->setText(title);
  this->edit_->setMinimumHeight(this->formRowHeight_);
  this->role_ = role;
}

void FormButtonElement::onChange(const QObject* receiver, const char* slot)
{
  QObject::connect(this->edit_, SIGNAL(clicked()), receiver, slot);
}
void FormButtonElement::addToButtonBox(QDialogButtonBox* buttonBox)
{
  buttonBox->addButton(this->edit_, this->role_);
}
void FormButtonElement::setEnabled(bool enabled)
{
  this->edit_->setEnabled(enabled);
}
