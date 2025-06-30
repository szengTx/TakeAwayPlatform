-- 用户表
CREATE TABLE applicant (
    user_id BIGINT  AUTO_INCREMENT PRIMARY KEY,
    user_name VARCHAR(50) NOT NULL,
    password VARCHAR(255) NOT NULL,
    email VARCHAR(20) NOT NULL UNIQUE,
    phone VARCHAR(20) NOT NULL UNIQUE,
    role ENUM('admin','merchant','customer') NOT NULL DEFAULT 'customer',
    avatar_url VARCHAR(255) ,
    create_at DATETIME 
)AUTO_INCREMENT=1000;

-- 商家表
CREATE TABLE merchants (
    merchant_id BIGINT NOT NULL PRIMARY KEY,
    user_id BIGINT NOT NULL UNIQUE,
    shop_name VARCHAR(100) NOT NULL,
    contact_phone VARCHAR(20) NOT NULL,
    shop_address VARCHAR(255) NOT NULL,
    shop_logo VARCHAR(255) ,
    description TEXT ,
    business_hours JSON ,
    delivery_fee DECIMAL(10,2) NOT NULL,
    min_order_amount DECIMAL(10,2) NOT NULL,
    status ENUM('pending','approved','rejected')  DEFAULT 'pending',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES applicant(user_id)
);

-- 菜品表
CREATE TABLE dishes (
    dish_id INT NOT NULL PRIMARY KEY,
    merchant_id BIGINT NOT NULL,
    name VARCHAR(255) NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    image_url VARCHAR(512) ,
    description TEXT ,
    stock INT(32) NOT NULL,
    category VARCHAR(100) NOT NULL,
    status ENUM('available','sold_out') NOT NULL,
    sales_count INT(32) ,
    created_at DATETIME NOT NULL,
    updated_at DATETIME NOT NULL,
    FOREIGN KEY (merchant_id) REFERENCES merchants(merchant_id)
);

-- 订单表
CREATE TABLE orders (
    order_id INT NOT NULL PRIMARY KEY,
    order_number VARCHAR(50) NOT NULL,
    user_id BIGINT NOT NULL,
    merchant_id BIGINT NOT NULL,
    total_amount DECIMAL(10,2) NOT NULL,
    status ENUM('unpaid','pending','preparing','delivering','completed','cancelled') NOT NULL,
    delivery_address JSON NOT NULL,
    contact_phone VARCHAR(20) NOT NULL,
    items JSON NOT NULL,
    created_at DATETIME NOT NULL,
    updated_at DATETIME NOT NULL,
    FOREIGN KEY (user_id) REFERENCES applicant(user_id),
    FOREIGN KEY (merchant_id) REFERENCES merchants(merchant_id)
);

-- 评价表
CREATE TABLE reviews (
    review_id INT NOT NULL PRIMARY KEY,
    order_id INT NOT NULL,
    user_id BIGINT NOT NULL,
    merchant_id BIGINT NOT NULL,
    rating TINYINT NOT NULL,
    content TEXT,
    images JSON,
    reply TEXT,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (order_id) REFERENCES orders(order_id),
    FOREIGN KEY (user_id) REFERENCES applicant(user_id),
    FOREIGN KEY (merchant_id) REFERENCES merchants(merchant_id)
);

-- 登录注册表
CREATE TABLE log_records (
    log_id INT NOT NULL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    action_type ENUM('register','login','logout','password_change') NOT NULL,
    status ENUM('success','failed') NOT NULL DEFAULT 'success',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES applicant(user_id)
);

-- 钱包账户表
CREATE TABLE wallet (
    wallet_id INT NOT NULL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    balance DECIMAL(10,2),
    status ENUM('active','frozen','closed') NOT NULL,
    created_at DATETIME NOT NULL,
    FOREIGN KEY (user_id) REFERENCES applicant(user_id)
);

-- 交易记录表
CREATE TABLE transactions (
    transaction_id INT NOT NULL PRIMARY KEY,
    wallet_id INT NOT NULL,
    order_id INT NOT NULL,
    amount DECIMAL(10,2),
    type ENUM('payment','refund','withdrawal','deposit') NOT NULL,
   
    status ENUM('processing','completed','failed') NOT NULL,
    created_at DATETIME NOT NULL,
    FOREIGN KEY (wallet_id) REFERENCES wallet(wallet_id),
    FOREIGN KEY (order_id) REFERENCES orders(order_id)
);

-- 充值记录表
CREATE TABLE recharge_record (
    recharge_id INT NOT NULL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    amount DECIMAL(10,2) NOT NULL,
   
    transaction_id INT NOT NULL,
    status ENUM('pending','completed','failed') NOT NULL,
    paid_at DATETIME NOT NULL,
    created_at DATETIME NOT NULL,
    FOREIGN KEY (user_id) REFERENCES applicant(user_id),
    FOREIGN KEY (transaction_id) REFERENCES transactions(transaction_id)
);
-- 购物车表
CREATE TABLE shopping_cart (
    cart_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    dish_id INT NOT NULL,
    quantity INT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES applicant(user_id),
    FOREIGN KEY (dish_id) REFERENCES dishes(dish_id),
    UNIQUE KEY (user_id, dish_id)
);