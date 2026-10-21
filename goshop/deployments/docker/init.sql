CREATE DATABASE IF NOT EXISTS goshop DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE goshop;

INSERT INTO categories (name, icon, parent_id, sort, created_at, updated_at) VALUES
('数码电子', '', 0, 1, NOW(), NOW()),
('服装鞋包', '', 0, 2, NOW(), NOW()),
('家居生活', '', 0, 3, NOW(), NOW()),
('食品饮料', '', 0, 4, NOW(), NOW()),
('图书音像', '', 0, 5, NOW(), NOW());
