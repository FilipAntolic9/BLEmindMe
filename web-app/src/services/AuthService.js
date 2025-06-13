import backendClient from "../api/axios";
import {AxiosError} from "axios";

export const login = async (username, password) => {
    try {
        const response = await backendClient.post(`/auth/login`, {username, password});
        return response.data;

    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }

        throw error;
    }
};

export const getCurrentUserInfo = async () => {
    try {
        const response = await backendClient.get(`/auth/user`);
        return response.data;

    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }

        throw error;
    }
};

export const createUser = async (email, firstName, lastName, phone, customerId) => {
    try {
        const authority = "CUSTOMER_USER";
        const customerIdObject = {
            id: customerId,
            entityType: "CUSTOMER"
        }

        const response = await backendClient.post(`/user?sendActivationMail=false`, {email, authority, firstName, lastName, phone, customerId: customerIdObject});
        return response.data;

    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }

        throw error;
    }
};

export const createCustomer = async (title, email, phone) => {
    try {
        const response = await backendClient.post(`/customer`, {title, email, phone});
        return response.data;

    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }

        throw error;
    }
};

export const getUserActivationLink = async (userId) => {
    try {
        const response = await backendClient.get(`/user/${userId}/activationLink`);
        return response.data;

    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }

        throw error;
    }
};
