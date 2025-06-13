import React from 'react';
import { createPortal } from 'react-dom';
import { FaCheck, FaTimes } from 'react-icons/fa';
import { IoWarning } from 'react-icons/io5';

const NotificationToast = ({ message, type, onClose }) => {
    const isSuccess = type === 'success';

    return createPortal(
        <div className={`fixed top-4 right-4 bg-[#202020] ${isSuccess ? 'text-white' : 'text-red-500'} p-4 rounded-lg shadow-lg z-50 flex items-center`}>
            <div className={`${isSuccess ? 'bg-[#c5f82a]' : ''} rounded-full p-1 mr-3`}>
                {isSuccess ? (
                    <FaCheck className="text-black" size={16} />
                ) : (
                    <IoWarning className="text-red-500" size={35} />
                )}
            </div>
            <span>{message}</span>
            <button
                onClick={onClose}
                className="ml-4 text-gray-400 hover:text-white"
            >
                <FaTimes size={16} />
            </button>
        </div>,
        document.body
    );
};

export default NotificationToast;