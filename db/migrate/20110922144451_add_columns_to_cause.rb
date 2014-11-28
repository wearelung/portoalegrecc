class AddColumnsToCause < ActiveRecord::Migration
  def self.up
    add_column :causes, :email, :string
    add_column :causes, :phone_no, :string
    add_column :causes, :cell_phone_no, :string
  end

  def self.down
    remove_column :causes, :email
    remove_column :causes, :phone_no
    remove_column :causes, :cell_phone_no
  end
end
